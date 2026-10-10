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
#include <array>
#include <cmath>
#include <filesystem>
#include <optional>
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
    // The shader's value with no map, for channels no slot param stands in for; -1 = a param does (any constant folds).
    std::array<int, 3> Neutral;
};

constexpr std::array<int, 3> kHasParam {-1, -1, -1};

const MapInfo kMaps[] = {
    {TerrainMap_BaseColor, "basecolor", &TerrainMaterialDesc::BaseColorPath, 3, true, kHasParam},
    {TerrainMap_Normal, "normal", &TerrainMaterialDesc::NormalPath, 3, false, {128, 128, 255}},
    {TerrainMap_Roughness, "roughness", &TerrainMaterialDesc::RoughnessPath, 1, false, kHasParam},
    {TerrainMap_Metallic, "metallic", &TerrainMaterialDesc::MetallicPath, 1, false, kHasParam},
    {TerrainMap_Height, "height", &TerrainMaterialDesc::HeightPath, 1, false, {128, 128, 128}},
    {TerrainMap_AO, "ao", &TerrainMaterialDesc::AOPath, 1, false, {255, 255, 255}},
    {TerrainMap_Emissive, "emissive", &TerrainMaterialDesc::EmissivePath, 3, true, kHasParam},
    {TerrainMap_Opacity, "opacity", &TerrainMaterialDesc::OpacityPath, 1, false, kHasParam},
    {TerrainMap_Mask, "mask", &TerrainMaterialDesc::MaskPath, 1, false, {0, 0, 0}},
};
constexpr size_t kMapCount = std::size(kMaps);
constexpr int kConstantTolerance = 2; // texel spread (0-255) still counted as one flat value

constexpr size_t MapIndex(TerrainMap bit) {
    for (size_t m = 0; m < kMapCount; m++)
        if (kMaps[m].Bit == bit)
            return m;
    return 0;
}

using MapConstants = std::array<std::optional<glm::vec3>, kMapCount>;

struct TerrainMaterialsData {
    std::array<TerrainMaterialDesc, kMaxTerrainMaterialSlots> Descs {};
    std::array<bool, kMaxTerrainMaterialSlots> Active {};
    std::array<uint32_t, kMaxTerrainMaterialSlots> Maps {}; // TerrainMap bits of the maps that loaded
    // Loaded maps that are one flat value on every layer: never sampled, their value uploaded instead (0-1, as stored).
    std::array<uint32_t, kMaxTerrainMaterialSlots> ConstantMaps {};
    std::array<std::array<glm::vec3, kMapCount>, kMaxTerrainMaterialSlots> ConstantValues {};
    // Source slot whose texture layer this slot samples (SetSlotShared), or -1 when it owns its own layer.
    std::array<int32_t, kMaxTerrainMaterialSlots> SharedFrom = [] {
        std::array<int32_t, kMaxTerrainMaterialSlots> none;
        none.fill(-1);
        return none;
    }();
    // Variant layers sit after the kMaxTerrainMaterialSlots slot layers: VariantBase is a slot's first one,
    // VariantCount its variants including the main maps (1 = none).
    std::array<uint32_t, kMaxTerrainMaterialSlots> VariantBase {};
    std::array<uint32_t, kMaxTerrainMaterialSlots> VariantCount = [] {
        std::array<uint32_t, kMaxTerrainMaterialSlots> one;
        one.fill(1);
        return one;
    }();
    uint32_t Resolution = 2048;
    float TriplanarSharpness = 4.0f;
    float TriplanarFaceNormal = 1.0f;
    TextureVariationSettings Variation;
    glm::vec3 WorldPeriod {0.0f};
    bool Dirty = true; // arrays rebuilt lazily on the next BindTextures, so registering N slots loads once
    std::array<Scope<TextureArray>, kArrayCount> Arrays;
};

TerrainMaterialsData s_Data;

uint32_t LayerOwner(uint32_t slot) {
    return s_Data.SharedFrom[slot] >= 0 ? (uint32_t)s_Data.SharedFrom[slot] : slot;
}

// The slot's flat maps (see ConstantMaps); none until pending slot changes have been loaded.
uint32_t ConstantMapsOf(uint32_t slot) {
    return s_Data.Dirty ? 0u : s_Data.ConstantMaps[LayerOwner(slot)] & s_Data.Maps[slot];
}

// Uniform names built once - UploadUniforms runs every frame per terrain shader.
struct SlotUniformNames {
    std::string BaseColorTint, Roughness, Metallic, Emissive, EmissiveStrength, Opacity, Scale, NormalStrength,
        HeightScale, Maps, Active, Layer, VariantBase, VariantCount;
};

const std::array<SlotUniformNames, kMaxTerrainMaterialSlots>& GetSlotUniformNames() {
    static const std::array<SlotUniformNames, kMaxTerrainMaterialSlots> names = [] {
        std::array<SlotUniformNames, kMaxTerrainMaterialSlots> result;
        for (uint32_t i = 0; i < kMaxTerrainMaterialSlots; i++) {
            std::string p = "u_TerrainSlots[" + std::to_string(i) + "].";
            result[i] = {p + "BaseColorTint",    p + "Roughness", p + "Metallic", p + "Emissive",
                         p + "EmissiveStrength", p + "Opacity",   p + "Scale",    p + "NormalStrength",
                         p + "HeightScale",      p + "Maps",      p + "Active",   p + "Layer",
                         p + "VariantBase",      p + "VariantCount"};
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

// The map's single value (0-1 per channel) if every texel is within kConstantTolerance of it.
std::optional<glm::vec3> ConstantValue(const std::vector<uint8_t>& pixels, int channels) {
    glm::vec3 value(0.0f);
    for (int c = 0; c < channels; c++) {
        uint8_t lo = 255, hi = 0;
        for (size_t i = c; i < pixels.size(); i += channels) {
            lo = std::min(lo, pixels[i]);
            hi = std::max(hi, pixels[i]);
            if (hi - lo > kConstantTolerance)
                return std::nullopt;
        }
        value[c] = (lo + hi) * 0.5f / 255.0f;
    }
    return channels == 1 ? glm::vec3(value.x) : value;
}

// Whether a flat map can go unsampled: a slot param stands in for it, or it equals the shader's no-map value.
bool CanFold(const MapInfo& map, const glm::vec3& value) {
    for (int c = 0; c < 3; c++)
        if (map.Neutral[c] >= 0 && std::abs(value[c] * 255.0f - (float)map.Neutral[c]) > kConstantTolerance)
            return false;
    return true;
}

bool SameConstant(const glm::vec3& a, const std::optional<glm::vec3>& b) {
    return b && glm::all(glm::lessThanEqual(glm::abs(a - *b), glm::vec3(kConstantTolerance / 255.0f)));
}

glm::vec3 SrgbToLinear(const glm::vec3& srgb) {
    glm::vec3 linear;
    for (int c = 0; c < 3; c++)
        linear[c] = srgb[c] <= 0.04045f ? srgb[c] / 12.92f : std::pow((srgb[c] + 0.055f) / 1.055f, 2.4f);
    return linear;
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

// One texture set into `layer` of every array it uses, from whichever of `maps` load; returns the maps that did.
// `constants` gets each loaded map's flat value, where it has one.
uint32_t UploadLayer(const TerrainMaterialDesc& desc, uint32_t maps, uint32_t layer, uint32_t size,
                     MapConstants& constants) {
    std::vector<uint8_t> loaded[kMapCount];
    constants = {};
    for (size_t m = 0; m < kMapCount; m++) {
        if (!(maps & kMaps[m].Bit))
            continue;
        loaded[m] = LoadMap(desc.*kMaps[m].Path, kMaps[m].Channels, kMaps[m].Srgb, size);
        if (loaded[m].empty())
            maps &= ~kMaps[m].Bit; // readable at SetSlot but not now - fall back to its default
        else
            constants[m] = ConstantValue(loaded[m], kMaps[m].Channels);
    }
    auto mapPixels = [&](TerrainMap bit) -> const std::vector<uint8_t>* {
        return &loaded[MapIndex(bit)];
    };
    auto setLayer = [&](ArrayKind kind, const std::vector<uint8_t>& pixels) {
        TextureArray& array = *s_Data.Arrays[(int)kind];
        if (layer < array.GetLayerCount() &&
            pixels.size() == (size_t)size * size * TextureArrayChannels(array.GetFormat()))
            array.SetLayer(layer, pixels.data());
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
    return maps;
}

// A variant missing a channel the main maps have borrows the main one, so its layer is never left empty.
TerrainMaterialDesc VariantWithFallbacks(const TerrainMaterialDesc& variant, const TerrainMaterialDesc& main) {
    TerrainMaterialDesc merged = variant;
    for (const MapInfo& map : kMaps)
        if ((merged.*map.Path).empty())
            merged.*map.Path = main.*map.Path;
    return merged;
}

// The slot's main layer (its slot index) plus each of its variants' layers.
// A map counts as constant only if every layer (main + variants) is flat at the same value.
void UploadSlot(uint32_t slot, uint32_t size) {
    const TerrainMaterialDesc& desc = s_Data.Descs[slot];
    MapConstants constants, variantConstants;
    s_Data.Maps[slot] = UploadLayer(desc, s_Data.Maps[slot], slot, size, constants);
    for (uint32_t v = 1; v < s_Data.VariantCount[slot]; v++) {
        UploadLayer(VariantWithFallbacks(desc.Variants[v - 1], desc), s_Data.Maps[slot],
                    s_Data.VariantBase[slot] + v - 1, size, variantConstants);
        for (size_t m = 0; m < kMapCount; m++)
            if (constants[m] && !SameConstant(*constants[m], variantConstants[m]))
                constants[m].reset();
    }
    s_Data.ConstantMaps[slot] = 0;
    for (size_t m = 0; m < kMapCount; m++) {
        if ((s_Data.Maps[slot] & kMaps[m].Bit) && constants[m] && CanFold(kMaps[m], *constants[m])) {
            s_Data.ConstantMaps[slot] |= kMaps[m].Bit;
            s_Data.ConstantValues[slot][m] = *constants[m];
            WK_CORE_INFO("TerrainMaterials - '{0}' is a flat value, not sampled", desc.*kMaps[m].Path);
        }
    }
}

void RebuildArrays() {
    s_Data.Dirty = false;

    // Each array holds layers 0..highest owning slot using it (sharers sample their source's layer), then the
    // variant layers; an array no slot uses is a 1x1 placeholder.
    auto ownsLayer = [](uint32_t slot) {
        return s_Data.Active[slot] && s_Data.SharedFrom[slot] < 0;
    };
    uint32_t nextVariantLayer = kMaxTerrainMaterialSlots;
    for (uint32_t slot = 0; slot < kMaxTerrainMaterialSlots; slot++) {
        uint32_t variants = ownsLayer(slot) ? (uint32_t)s_Data.Descs[slot].Variants.size() : 0;
        variants = std::min(variants, kMaxTerrainVariants - 1);
        s_Data.VariantBase[slot] = nextVariantLayer;
        s_Data.VariantCount[slot] = 1 + variants;
        nextVariantLayer += variants;
    }
    for (int a = 0; a < kArrayCount; a++) {
        uint32_t layers = 0;
        for (uint32_t slot = 0; slot < kMaxTerrainMaterialSlots; slot++) {
            if (!ownsLayer(slot) || !(s_Data.Maps[slot] & kArrays[a].Maps))
                continue;
            layers = std::max(layers, slot + 1);
            if (s_Data.VariantCount[slot] > 1)
                layers = std::max(layers, s_Data.VariantBase[slot] + s_Data.VariantCount[slot] - 1);
        }
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

namespace {

// The map paths alone (no variant search) for `<dir>/<name>_<channel>.png`.
TerrainMaterialDesc MapsFromDirectory(const std::string& dir, const std::string& name) {
    TerrainMaterialDesc desc;
    for (const MapInfo& map : kMaps) {
        std::filesystem::path path = std::filesystem::path(dir) / (name + "_" + map.Suffix + ".png");
        if (std::filesystem::exists(path))
            desc.*map.Path = path.string();
    }
    return desc;
}

} // namespace

TerrainMaterialDesc TerrainMaterialDesc::FromDirectory(const std::string& dir, const std::string& name) {
    TerrainMaterialDesc desc = MapsFromDirectory(dir, name);
    for (uint32_t n = 1; n < kMaxTerrainVariants; n++) {
        TerrainMaterialDesc variant = MapsFromDirectory(dir, name + "_v" + std::to_string(n));
        if (variant.BaseColorPath.empty())
            break;
        desc.Variants.push_back(std::move(variant));
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
    SyncShaderDefines();
}

void TerrainMaterials::SyncShaderDefines() {
    const TextureVariationSettings& v = s_Data.Variation;
    auto on = [&](bool feature) {
        return v.Enabled && feature ? 1 : 0;
    };
    Shader::SetGlobalDefine("TEXVAR_ENABLED", on(true));
    Shader::SetGlobalDefine("TEXVAR_TRANSFORM",
                            on(v.Transform.Enabled && (v.Transform.Rotation || v.Transform.Mirror)));
    Shader::SetGlobalDefine("TEXVAR_VARIANTS", on(v.Variants.Enabled));
    Shader::SetGlobalDefine("TEXVAR_MACRO", on(v.Macro.Enabled));
    Shader::SetGlobalDefine("TEXVAR_DETAIL", on(v.Detail.Enabled));
    Shader::SetGlobalDefine("TEXVAR_MATERIAL", on(v.Material.Enabled));
    Shader::SetGlobalDefine("TEXVAR_DEBUG_VIEW", v.Enabled ? (int)v.View : 0);

    uint32_t sampled = 0;
    for (uint32_t slot = 0; slot < kMaxTerrainMaterialSlots; slot++)
        if (s_Data.Active[slot])
            sampled |= s_Data.Maps[slot] & ~ConstantMapsOf(slot);
    Shader::SetGlobalDefine("TERRAIN_MAPS_USED", (int)sampled);
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

TextureVariationSettings& TerrainMaterials::GetVariationSettings() {
    return s_Data.Variation;
}

void TerrainMaterials::SetWorldPeriod(const glm::vec3& period) {
    s_Data.WorldPeriod = glm::max(period, glm::vec3(0.0f));
}

glm::vec3 TerrainMaterials::GetWorldPeriod() {
    return s_Data.WorldPeriod;
}

uint32_t TerrainMaterials::GetSlotVariantCount(uint32_t slot) {
    if (slot >= kMaxTerrainMaterialSlots)
        return 1;
    int32_t source = s_Data.SharedFrom[slot];
    return s_Data.VariantCount[source >= 0 ? source : slot];
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
        uint32_t owner = LayerOwner(i);

        // A flat map's value replaces the param it would have overridden (the base color map multiplies the tint).
        uint32_t constant = ConstantMapsOf(i);
        const auto& value = s_Data.ConstantValues[owner];
        auto folded = [&](TerrainMap bit) {
            return (constant & bit) != 0;
        };
        glm::vec3 tint = desc.BaseColorTint;
        if (folded(TerrainMap_BaseColor))
            tint *= SrgbToLinear(value[MapIndex(TerrainMap_BaseColor)]);
        float roughness = folded(TerrainMap_Roughness) ? value[MapIndex(TerrainMap_Roughness)].x : desc.Roughness;
        float metallic = folded(TerrainMap_Metallic) ? value[MapIndex(TerrainMap_Metallic)].x : desc.Metallic;
        glm::vec3 emissive =
            folded(TerrainMap_Emissive) ? SrgbToLinear(value[MapIndex(TerrainMap_Emissive)]) : desc.EmissiveColor;
        float opacity = folded(TerrainMap_Opacity) ? value[MapIndex(TerrainMap_Opacity)].x : desc.Opacity;

        shader->SetVec3(names[i].BaseColorTint, tint);
        shader->SetFloat(names[i].Roughness, roughness);
        shader->SetFloat(names[i].Metallic, metallic);
        shader->SetVec3(names[i].Emissive, emissive);
        shader->SetFloat(names[i].EmissiveStrength, desc.EmissiveStrength);
        shader->SetFloat(names[i].Opacity, opacity);
        shader->SetFloat(names[i].Scale, desc.TextureScale);
        shader->SetFloat(names[i].NormalStrength, desc.NormalStrength);
        shader->SetFloat(names[i].HeightScale, desc.HeightScale);
        shader->SetInt(names[i].Maps, (int)(s_Data.Maps[i] & ~constant));
        shader->SetInt(names[i].Active, s_Data.Active[i] ? 1 : 0);
        shader->SetInt(names[i].Layer, (int)owner);
        shader->SetInt(names[i].VariantBase, (int)s_Data.VariantBase[owner]);
        shader->SetInt(names[i].VariantCount, (int)s_Data.VariantCount[owner]);
    }

    // Texture repetition mitigation parameters - the on/off switches are defines, see SyncShaderDefines.
    const TextureVariationSettings& v = s_Data.Variation;
    shader->SetVec3("u_TexVarPeriod", s_Data.WorldPeriod);
    shader->SetFloat("u_TexVarTransformScale", std::max(v.Transform.Scale, 0.01f));
    shader->SetFloat("u_TexVarEdgeBlend", std::clamp(v.Transform.EdgeBlend, 0.0f, 0.5f));
    shader->SetInt("u_TexVarRotation", v.Transform.Rotation ? 1 : 0);
    shader->SetInt("u_TexVarMirror", v.Transform.Mirror ? 1 : 0);
    shader->SetInt("u_TexVarTransformSeed", (int)v.Transform.Seed);
    shader->SetInt("u_TexVarVariantCount", std::clamp(v.Variants.Count, 1, (int)kMaxTerrainVariants));
    shader->SetFloat("u_TexVarVariantScale", std::max(v.Variants.Scale, 0.01f));
    shader->SetInt("u_TexVarVariantSeed", (int)v.Variants.Seed);
    shader->SetFloat("u_TexVarMacroScale", std::max(v.Macro.Scale, 0.01f));
    shader->SetFloat("u_TexVarMacroStrength", v.Macro.Strength);
    shader->SetFloat("u_TexVarMacroContrast", v.Macro.Contrast);
    shader->SetInt("u_TexVarMacroSeed", (int)v.Macro.Seed);
    shader->SetFloat("u_TexVarDetailScale", std::max(v.Detail.Scale, 0.001f));
    shader->SetFloat("u_TexVarDetailStrength", v.Detail.Strength);
    shader->SetInt("u_TexVarDetailChannels",
                   (v.Detail.BaseColor ? 1 : 0) | (v.Detail.Normal ? 2 : 0) | (v.Detail.Roughness ? 4 : 0));
    shader->SetFloat("u_TexVarMaterialScale", std::max(v.Material.Scale, 0.01f));
    shader->SetFloat("u_TexVarMaterialAlbedo", v.Material.Albedo);
    shader->SetFloat("u_TexVarMaterialRoughness", v.Material.Roughness);
    shader->SetFloat("u_TexVarMaterialNormal", v.Material.Normal);
    shader->SetInt("u_TexVarMaterialSeed", (int)v.Material.Seed);
}

} // namespace Wankel
