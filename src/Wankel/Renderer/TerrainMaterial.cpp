#include "wkpch.h"
#include "TerrainMaterial.h"

#include "Shader.h"
#include "TextureArray.h"

#include <glad/gl.h>

// Their SSE paths pass non-constant shift immediates that newer GCC rejects outright.
#define STBI_NO_SIMD
#define STBIR_NO_SIMD
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

#include <algorithm>
#include <filesystem>
#include <vector>

namespace Wankel {

namespace {

// Five arrays, one per packing: scalar channels share RGBA8/RG8 layers so they cost one fetch, not four.
enum class ArrayKind : uint8_t { BaseColor, Normal, Surface, OpacityMask, Emissive, Count };
constexpr int kArrayCount = (int)ArrayKind::Count;

struct ArrayInfo {
    const char* Uniform;
    uint32_t Unit;
    TextureArrayFormat Format;
    uint32_t Maps; // TerrainMap bits stored in this array - a slot needs a layer only if it has one of them
};

constexpr uint32_t kSurfaceMaps = TerrainMap_AO | TerrainMap_Roughness | TerrainMap_Metallic | TerrainMap_Height;
constexpr uint32_t kOpacityMaskMaps = TerrainMap_Opacity | TerrainMap_Mask;

constexpr ArrayInfo kArrays[kArrayCount] = {
    {"u_TerrainBaseColor", 1, TextureArrayFormat::SRGB8, TerrainMap_BaseColor},
    {"u_TerrainNormal", 2, TextureArrayFormat::RGB8, TerrainMap_Normal},
    {"u_TerrainSurface", 3, TextureArrayFormat::RGBA8, kSurfaceMaps}, // AO, roughness, metallic, height
    {"u_TerrainOpacityMask", 4, TextureArrayFormat::RG8, kOpacityMaskMaps},
    {"u_TerrainEmissive", 5, TextureArrayFormat::SRGB8, TerrainMap_Emissive},
};

struct MapInfo {
    TerrainMap Bit;
    const char* Suffix; // file naming convention: <name>_<Suffix>.png
    std::string TerrainMaterialDesc::* Path;
    int Channels;
    bool Srgb;
};

const MapInfo kMaps[] = {
    {TerrainMap_BaseColor, "basecolor", &TerrainMaterialDesc::BaseColorPath, 3, true},
    {TerrainMap_Normal, "normal", &TerrainMaterialDesc::NormalPath, 3, false},
    {TerrainMap_Roughness, "roughness", &TerrainMaterialDesc::RoughnessPath, 1, false},
    {TerrainMap_Metallic, "metallic", &TerrainMaterialDesc::MetallicPath, 1, false},
    {TerrainMap_Height, "height", &TerrainMaterialDesc::HeightPath, 1, false},
    {TerrainMap_AO, "ao", &TerrainMaterialDesc::AOPath, 1, false},
    {TerrainMap_Emissive, "emissive", &TerrainMaterialDesc::EmissivePath, 3, true},
    {TerrainMap_Opacity, "opacity", &TerrainMaterialDesc::OpacityPath, 1, false},
    {TerrainMap_Mask, "mask", &TerrainMaterialDesc::MaskPath, 1, false},
};

struct TerrainMaterialsData {
    std::array<TerrainMaterialDesc, kMaxTerrainMaterialSlots> Descs {};
    std::array<bool, kMaxTerrainMaterialSlots> Active {};
    std::array<uint32_t, kMaxTerrainMaterialSlots> Maps {}; // TerrainMap bits of the maps that loaded
    // Source slot whose texture layer this slot samples (SetSlotShared), or -1 when it owns its own layer.
    std::array<int32_t, kMaxTerrainMaterialSlots> SharedFrom = [] {
        std::array<int32_t, kMaxTerrainMaterialSlots> none;
        none.fill(-1);
        return none;
    }();
    uint32_t Resolution = 2048;
    float TriplanarSharpness = 4.0f;
    float TriplanarFaceNormal = 1.0f;
    bool Dirty = true; // arrays rebuilt lazily on the next BindTextures, so registering N slots loads once
    std::array<Scope<TextureArray>, kArrayCount> Arrays;
};

TerrainMaterialsData s_Data;

// Uniform names built once - UploadUniforms runs every frame per terrain shader.
struct SlotUniformNames {
    std::string BaseColorTint, Roughness, Metallic, Emissive, EmissiveStrength, Opacity, Scale, NormalStrength,
        HeightScale, Maps, Active, Layer;
};

const std::array<SlotUniformNames, kMaxTerrainMaterialSlots>& GetSlotUniformNames() {
    static const std::array<SlotUniformNames, kMaxTerrainMaterialSlots> names = [] {
        std::array<SlotUniformNames, kMaxTerrainMaterialSlots> result;
        for (uint32_t i = 0; i < kMaxTerrainMaterialSlots; i++) {
            std::string p = "u_TerrainSlots[" + std::to_string(i) + "].";
            result[i] = {p + "BaseColorTint",    p + "Roughness", p + "Metallic", p + "Emissive",
                         p + "EmissiveStrength", p + "Opacity",   p + "Scale",    p + "NormalStrength",
                         p + "HeightScale",      p + "Maps",      p + "Active",   p + "Layer"};
        }
        return result;
    }();
    return names;
}

// One map at size x size, `channels` bytes per texel; empty on failure. Color data resizes in sRGB space.
std::vector<uint8_t> LoadMap(const std::string& path, int channels, bool srgb, uint32_t size) {
    int width = 0, height = 0, fileChannels = 0;

    // Bottom-up rows so image +Y matches UV +V - keeps the OpenGL-convention normal map's green channel correct.
    stbi_set_flip_vertically_on_load(1);
    stbi_uc* source = stbi_load(path.c_str(), &width, &height, &fileChannels, channels);
    stbi_set_flip_vertically_on_load(0);

    if (!source) {
        WK_CORE_ERROR("TerrainMaterials - failed to load '{0}': {1}", path, stbi_failure_reason());
        return {};
    }

    std::vector<uint8_t> pixels((size_t)size * size * channels);
    auto layout = channels == 1 ? STBIR_1CHANNEL : STBIR_RGB;
    if (srgb)
        stbir_resize_uint8_srgb(source, width, height, 0, pixels.data(), (int)size, (int)size, 0, layout);
    else
        stbir_resize_uint8_linear(source, width, height, 0, pixels.data(), (int)size, (int)size, 0, layout);

    stbi_image_free(source);
    return pixels;
}

// Interleaves single-channel maps into one multi-channel layer; a missing channel gets its default value.
std::vector<uint8_t> PackChannels(const std::vector<const std::vector<uint8_t>*>& channels,
                                  const std::vector<uint8_t>& defaults, uint32_t size) {
    size_t texels = (size_t)size * size;
    size_t count = channels.size();
    std::vector<uint8_t> packed(texels * count);
    for (size_t c = 0; c < count; c++) {
        const std::vector<uint8_t>* src = channels[c];
        for (size_t t = 0; t < texels; t++)
            packed[t * count + c] = src && !src->empty() ? (*src)[t] : defaults[c];
    }
    return packed;
}

// Each slot's layer for every array it uses, built from whichever of its maps exist.
void UploadSlot(uint32_t slot, uint32_t size) {
    const TerrainMaterialDesc& desc = s_Data.Descs[slot];
    uint32_t maps = s_Data.Maps[slot];

    std::vector<uint8_t> loaded[std::size(kMaps)];
    for (size_t m = 0; m < std::size(kMaps); m++) {
        if (!(maps & kMaps[m].Bit))
            continue;
        loaded[m] = LoadMap(desc.*kMaps[m].Path, kMaps[m].Channels, kMaps[m].Srgb, size);
        if (loaded[m].empty())
            maps &= ~kMaps[m].Bit; // readable at SetSlot but not now - fall back to its default
    }
    s_Data.Maps[slot] = maps;
    auto mapPixels = [&](TerrainMap bit) -> const std::vector<uint8_t>* {
        for (size_t m = 0; m < std::size(kMaps); m++)
            if (kMaps[m].Bit == bit)
                return &loaded[m];
        return nullptr;
    };
    auto setLayer = [&](ArrayKind kind, const std::vector<uint8_t>& pixels) {
        TextureArray& array = *s_Data.Arrays[(int)kind];
        if (slot < array.GetLayerCount() &&
            pixels.size() == (size_t)size * size * TextureArrayChannels(array.GetFormat()))
            array.SetLayer(slot, pixels.data());
    };

    if (maps & TerrainMap_BaseColor)
        setLayer(ArrayKind::BaseColor, *mapPixels(TerrainMap_BaseColor));
    if (maps & TerrainMap_Normal)
        setLayer(ArrayKind::Normal, *mapPixels(TerrainMap_Normal));
    if (maps & TerrainMap_Emissive)
        setLayer(ArrayKind::Emissive, *mapPixels(TerrainMap_Emissive));
    if (maps & kSurfaceMaps) {
        // Defaults only ever read for channels whose map is missing - the shader ignores those anyway.
        setLayer(ArrayKind::Surface, PackChannels({mapPixels(TerrainMap_AO), mapPixels(TerrainMap_Roughness),
                                                   mapPixels(TerrainMap_Metallic), mapPixels(TerrainMap_Height)},
                                                  {255, 255, 0, 128}, size));
    }
    if (maps & kOpacityMaskMaps) {
        setLayer(ArrayKind::OpacityMask,
                 PackChannels({mapPixels(TerrainMap_Opacity), mapPixels(TerrainMap_Mask)}, {255, 0}, size));
    }
}

void RebuildArrays() {
    s_Data.Dirty = false;

    // Each array holds layers 0..highest owning slot using it (sharers sample their source's layer); an
    // array no slot uses is a 1x1 placeholder.
    auto ownsLayer = [](uint32_t slot) {
        return s_Data.Active[slot] && s_Data.SharedFrom[slot] < 0;
    };
    for (int a = 0; a < kArrayCount; a++) {
        uint32_t layers = 0;
        for (uint32_t slot = 0; slot < kMaxTerrainMaterialSlots; slot++)
            if (ownsLayer(slot) && (s_Data.Maps[slot] & kArrays[a].Maps))
                layers = slot + 1;
        uint32_t size = layers > 0 ? s_Data.Resolution : 1;
        s_Data.Arrays[a] = CreateScope<TextureArray>(size, std::max(layers, 1u), kArrays[a].Format);
    }

    for (uint32_t slot = 0; slot < kMaxTerrainMaterialSlots; slot++)
        if (ownsLayer(slot))
            UploadSlot(slot, s_Data.Resolution);
    // UploadSlot may have dropped maps that failed to load - sharers follow their source.
    for (uint32_t slot = 0; slot < kMaxTerrainMaterialSlots; slot++)
        if (s_Data.Active[slot] && s_Data.SharedFrom[slot] >= 0)
            s_Data.Maps[slot] = s_Data.Maps[s_Data.SharedFrom[slot]];

    for (const Scope<TextureArray>& array : s_Data.Arrays)
        array->GenerateMips();
}

} // namespace

TerrainMaterialDesc TerrainMaterialDesc::FromDirectory(const std::string& dir, const std::string& name) {
    TerrainMaterialDesc desc;
    for (const MapInfo& map : kMaps) {
        std::filesystem::path path = std::filesystem::path(dir) / (name + "_" + map.Suffix + ".png");
        if (std::filesystem::exists(path))
            desc.*map.Path = path.string();
    }
    return desc;
}

namespace {

// Deactivates every slot sampling `source`'s layer - they'd otherwise read a missing layer.
void ClearSharersOf(uint32_t source) {
    for (uint32_t slot = 0; slot < kMaxTerrainMaterialSlots; slot++) {
        if (s_Data.SharedFrom[slot] != (int32_t)source)
            continue;
        s_Data.Active[slot] = false;
        s_Data.Maps[slot] = 0;
        s_Data.Descs[slot] = {};
        s_Data.SharedFrom[slot] = -1;
    }
}

} // namespace

void TerrainMaterials::Init() {
    RebuildArrays();
}

void TerrainMaterials::Shutdown() {
    for (Scope<TextureArray>& array : s_Data.Arrays)
        array.reset();
    s_Data.Active.fill(false);
    s_Data.Maps.fill(0);
    s_Data.SharedFrom.fill(-1);
}

bool TerrainMaterials::SetSlot(uint32_t slot, const TerrainMaterialDesc& desc) {
    if (slot >= kMaxTerrainMaterialSlots) {
        WK_CORE_ERROR("TerrainMaterials::SetSlot - slot {0} out of range (max {1})", slot, kMaxTerrainMaterialSlots);
        return false;
    }

    // Header-only probe now so the slot's maps are known immediately; pixels load on the next rebuild.
    bool allLoaded = true;
    uint32_t maps = 0;
    for (const MapInfo& map : kMaps) {
        const std::string& path = desc.*map.Path;
        if (path.empty())
            continue;
        int w = 0, h = 0, n = 0;
        if (stbi_info(path.c_str(), &w, &h, &n)) {
            maps |= map.Bit;
        } else {
            WK_CORE_ERROR("TerrainMaterials - can't read '{0}' ({1}), using the {2} default instead", path,
                          stbi_failure_reason(), map.Suffix);
            allLoaded = false;
        }
    }

    // Any slots already sharing this one keep doing so - RebuildArrays re-mirrors their maps.
    s_Data.Descs[slot] = desc;
    s_Data.Maps[slot] = maps;
    s_Data.Active[slot] = true;
    s_Data.SharedFrom[slot] = -1;
    s_Data.Dirty = true;
    return allLoaded;
}

bool TerrainMaterials::SetSlotShared(uint32_t slot, uint32_t sourceSlot, const TerrainMaterialDesc& params) {
    if (slot >= kMaxTerrainMaterialSlots || sourceSlot >= kMaxTerrainMaterialSlots || slot == sourceSlot) {
        WK_CORE_ERROR("TerrainMaterials::SetSlotShared - invalid slot {0} / source {1}", slot, sourceSlot);
        return false;
    }
    if (!s_Data.Active[sourceSlot] || s_Data.SharedFrom[sourceSlot] >= 0) {
        WK_CORE_ERROR("TerrainMaterials::SetSlotShared - source slot {0} must be an active SetSlot slot", sourceSlot);
        return false;
    }

    if (s_Data.Active[slot] && s_Data.SharedFrom[slot] < 0) {
        ClearSharersOf(slot);
        s_Data.Dirty = true; // its own layer goes away
    }

    // Keep the source's paths so the slot's desc still describes what it actually samples.
    TerrainMaterialDesc desc = params;
    for (const MapInfo& map : kMaps)
        desc.*map.Path = s_Data.Descs[sourceSlot].*map.Path;

    s_Data.Descs[slot] = desc;
    s_Data.Maps[slot] = s_Data.Maps[sourceSlot];
    s_Data.Active[slot] = true;
    s_Data.SharedFrom[slot] = (int32_t)sourceSlot;
    return true;
}

void TerrainMaterials::ClearSlot(uint32_t slot) {
    if (slot >= kMaxTerrainMaterialSlots || !s_Data.Active[slot])
        return;
    if (s_Data.SharedFrom[slot] < 0) {
        ClearSharersOf(slot);
        s_Data.Dirty = true;
    }
    s_Data.Active[slot] = false;
    s_Data.Maps[slot] = 0;
    s_Data.Descs[slot] = {};
    s_Data.SharedFrom[slot] = -1;
}

bool TerrainMaterials::IsSlotActive(uint32_t slot) {
    return slot < kMaxTerrainMaterialSlots && s_Data.Active[slot];
}

uint32_t TerrainMaterials::GetSlotMaps(uint32_t slot) {
    return slot < kMaxTerrainMaterialSlots ? s_Data.Maps[slot] : 0;
}

const TerrainMaterialDesc& TerrainMaterials::GetSlotDesc(uint32_t slot) {
    return s_Data.Descs[std::min(slot, kMaxTerrainMaterialSlots - 1)];
}

void TerrainMaterials::SetSlotParams(uint32_t slot, const TerrainMaterialDesc& params) {
    if (slot >= kMaxTerrainMaterialSlots)
        return;
    TerrainMaterialDesc& desc = s_Data.Descs[slot];
    desc.BaseColorTint = params.BaseColorTint;
    desc.Roughness = params.Roughness;
    desc.Metallic = params.Metallic;
    desc.EmissiveColor = params.EmissiveColor;
    desc.EmissiveStrength = params.EmissiveStrength;
    desc.Opacity = params.Opacity;
    desc.TextureScale = params.TextureScale;
    desc.NormalStrength = params.NormalStrength;
    desc.HeightScale = params.HeightScale;
}

void TerrainMaterials::SetResolution(uint32_t resolution) {
    resolution = std::clamp(resolution, 1u, 8192u);
    if (resolution == s_Data.Resolution)
        return;
    s_Data.Resolution = resolution;
    s_Data.Dirty = true;
}

uint32_t TerrainMaterials::GetResolution() {
    return s_Data.Resolution;
}

void TerrainMaterials::SetTriplanarSharpness(float sharpness) {
    s_Data.TriplanarSharpness = std::max(sharpness, 1.0f);
}

float TerrainMaterials::GetTriplanarSharpness() {
    return s_Data.TriplanarSharpness;
}

void TerrainMaterials::SetTriplanarFaceNormal(float blend) {
    s_Data.TriplanarFaceNormal = std::clamp(blend, 0.0f, 1.0f);
}

float TerrainMaterials::GetTriplanarFaceNormal() {
    return s_Data.TriplanarFaceNormal;
}

void TerrainMaterials::BindTextures() {
    if (s_Data.Dirty)
        RebuildArrays();
    for (int a = 0; a < kArrayCount; a++)
        s_Data.Arrays[a]->Bind(kArrays[a].Unit);
    glActiveTexture(GL_TEXTURE0); // every other texture user assumes unit 0 is active
}

void TerrainMaterials::UploadUniforms(Shader* shader) {
    for (const ArrayInfo& array : kArrays)
        shader->SetInt(array.Uniform, (int)array.Unit);
    shader->SetFloat("u_TriplanarSharpness", s_Data.TriplanarSharpness);
    shader->SetFloat("u_TriplanarFaceNormal", s_Data.TriplanarFaceNormal);

    const auto& names = GetSlotUniformNames();
    for (uint32_t i = 0; i < kMaxTerrainMaterialSlots; i++) {
        const TerrainMaterialDesc& desc = s_Data.Descs[i];
        shader->SetVec3(names[i].BaseColorTint, desc.BaseColorTint);
        shader->SetFloat(names[i].Roughness, desc.Roughness);
        shader->SetFloat(names[i].Metallic, desc.Metallic);
        shader->SetVec3(names[i].Emissive, desc.EmissiveColor);
        shader->SetFloat(names[i].EmissiveStrength, desc.EmissiveStrength);
        shader->SetFloat(names[i].Opacity, desc.Opacity);
        shader->SetFloat(names[i].Scale, desc.TextureScale);
        shader->SetFloat(names[i].NormalStrength, desc.NormalStrength);
        shader->SetFloat(names[i].HeightScale, desc.HeightScale);
        shader->SetInt(names[i].Maps, (int)s_Data.Maps[i]);
        shader->SetInt(names[i].Active, s_Data.Active[i] ? 1 : 0);
        shader->SetInt(names[i].Layer, s_Data.SharedFrom[i] >= 0 ? s_Data.SharedFrom[i] : (int)i);
    }
}

} // namespace Wankel
