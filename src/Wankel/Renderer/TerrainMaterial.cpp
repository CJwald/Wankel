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
#include <vector>

namespace Wankel {

namespace {

constexpr uint32_t kAlbedoUnit = 1;
constexpr uint32_t kNormalUnit = 2;
constexpr uint32_t kRoughnessUnit = 3;

enum class MapKind : uint8_t { Albedo, Normal, Roughness };

struct TerrainMaterialsData {
    std::array<TerrainMaterialDesc, kMaxTerrainMaterialSlots> Descs {};
    std::array<bool, kMaxTerrainMaterialSlots> Active {};
    uint32_t Resolution = 2048;
    float TriplanarSharpness = 4.0f;
    float TriplanarFaceNormal = 1.0f;
    Scope<TextureArray> AlbedoArray;
    Scope<TextureArray> NormalArray;
    Scope<TextureArray> RoughnessArray;
};

TerrainMaterialsData s_Data;

// Uniform names built once - UploadUniforms runs every frame per terrain shader.
struct SlotUniformNames {
    std::string Tint, Roughness, Metallic, Scale, NormalStrength, Active;
};

const std::array<SlotUniformNames, kMaxTerrainMaterialSlots>& GetSlotUniformNames() {
    static const std::array<SlotUniformNames, kMaxTerrainMaterialSlots> names = [] {
        std::array<SlotUniformNames, kMaxTerrainMaterialSlots> result;
        for (uint32_t i = 0; i < kMaxTerrainMaterialSlots; i++) {
            std::string prefix = "u_TerrainSlots[" + std::to_string(i) + "].";
            result[i] = {prefix + "Tint",  prefix + "Roughness",      prefix + "Metallic",
                         prefix + "Scale", prefix + "NormalStrength", prefix + "Active"};
        }
        return result;
    }();
    return names;
}

int ChannelsFor(MapKind kind) {
    return kind == MapKind::Roughness ? 1 : 3;
}

std::vector<uint8_t> MakeFallbackMap(MapKind kind, uint32_t size) {
    size_t texels = (size_t)size * size;
    if (kind == MapKind::Roughness)
        return std::vector<uint8_t>(texels, 255);

    std::vector<uint8_t> pixels(texels * 3);
    const uint8_t fill[3] = {255, 255, 255};
    const uint8_t flatNormal[3] = {128, 128, 255};
    const uint8_t* texel = kind == MapKind::Normal ? flatNormal : fill;
    for (size_t i = 0; i < texels; i++)
        std::copy(texel, texel + 3, pixels.begin() + (std::ptrdiff_t)(i * 3));
    return pixels;
}

// Empty result means the file failed to load; an empty path returns the fallback map instead.
std::vector<uint8_t> LoadMap(const std::string& path, MapKind kind, uint32_t size) {
    if (path.empty())
        return MakeFallbackMap(kind, size);

    int channels = ChannelsFor(kind);
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
    if (kind == MapKind::Albedo)
        stbir_resize_uint8_srgb(source, width, height, 0, pixels.data(), (int)size, (int)size, 0, layout);
    else
        stbir_resize_uint8_linear(source, width, height, 0, pixels.data(), (int)size, (int)size, 0, layout);

    stbi_image_free(source);
    return pixels;
}

TextureArray* ArrayFor(MapKind kind) {
    switch (kind) {
        case MapKind::Albedo:
            return s_Data.AlbedoArray.get();
        case MapKind::Normal:
            return s_Data.NormalArray.get();
        case MapKind::Roughness:
        default:
            return s_Data.RoughnessArray.get();
    }
}

const std::string& PathFor(const TerrainMaterialDesc& desc, MapKind kind) {
    switch (kind) {
        case MapKind::Albedo:
            return desc.AlbedoPath;
        case MapKind::Normal:
            return desc.NormalPath;
        case MapKind::Roughness:
        default:
            return desc.RoughnessPath;
    }
}

constexpr MapKind kMapKinds[] = {MapKind::Albedo, MapKind::Normal, MapKind::Roughness};

bool AnySlotActive() {
    return std::any_of(s_Data.Active.begin(), s_Data.Active.end(), [](bool a) { return a; });
}

// Inactive slots get fallback maps - never sampled (their weights are always 0), but keeps every layer defined.
bool UploadSlot(uint32_t slot, const TerrainMaterialDesc& desc, bool active) {
    uint32_t size = s_Data.AlbedoArray->GetSize();
    std::vector<uint8_t> maps[3];
    for (int k = 0; k < 3; k++) {
        maps[k] =
            active ? LoadMap(PathFor(desc, kMapKinds[k]), kMapKinds[k], size) : MakeFallbackMap(kMapKinds[k], size);
        if (maps[k].empty())
            return false;
    }
    for (int k = 0; k < 3; k++)
        ArrayFor(kMapKinds[k])->SetLayer(slot, maps[k].data());
    return true;
}

void GenerateAllMips() {
    for (MapKind kind : kMapKinds)
        ArrayFor(kind)->GenerateMips();
}

} // namespace

void TerrainMaterials::Init() {
    RebuildArrays();
}

void TerrainMaterials::Shutdown() {
    s_Data.AlbedoArray.reset();
    s_Data.NormalArray.reset();
    s_Data.RoughnessArray.reset();
    s_Data.Active.fill(false);
}

void TerrainMaterials::RebuildArrays() {
    // No active slot -> 1x1 layers, so an untextured app pays no VRAM for this system.
    uint32_t size = AnySlotActive() ? s_Data.Resolution : 1;
    s_Data.AlbedoArray = CreateScope<TextureArray>(size, kMaxTerrainMaterialSlots, TextureArrayFormat::RGB8);
    s_Data.NormalArray = CreateScope<TextureArray>(size, kMaxTerrainMaterialSlots, TextureArrayFormat::RGB8);
    s_Data.RoughnessArray = CreateScope<TextureArray>(size, kMaxTerrainMaterialSlots, TextureArrayFormat::R8);

    for (uint32_t slot = 0; slot < kMaxTerrainMaterialSlots; slot++) {
        if (s_Data.Active[slot] && !UploadSlot(slot, s_Data.Descs[slot], true)) {
            WK_CORE_ERROR("TerrainMaterials - slot {0} failed to reload, falling back to flat maps", slot);
            UploadSlot(slot, s_Data.Descs[slot], false);
        } else if (!s_Data.Active[slot]) {
            UploadSlot(slot, s_Data.Descs[slot], false);
        }
    }
    GenerateAllMips();
}

bool TerrainMaterials::SetSlot(uint32_t slot, const TerrainMaterialDesc& desc) {
    if (slot >= kMaxTerrainMaterialSlots) {
        WK_CORE_ERROR("TerrainMaterials::SetSlot - slot {0} out of range (max {1})", slot, kMaxTerrainMaterialSlots);
        return false;
    }

    bool needsResize = !s_Data.AlbedoArray || s_Data.AlbedoArray->GetSize() != s_Data.Resolution;
    s_Data.Descs[slot] = desc;

    if (needsResize) {
        // Validate this slot's files first so a bad path doesn't cost a full-resolution rebuild.
        for (MapKind kind : kMapKinds) {
            const std::string& path = PathFor(desc, kind);
            int w = 0, h = 0, n = 0;
            if (!path.empty() && !stbi_info(path.c_str(), &w, &h, &n)) {
                WK_CORE_ERROR("TerrainMaterials - failed to read '{0}': {1}", path, stbi_failure_reason());
                s_Data.Active[slot] = false;
                return false;
            }
        }
        s_Data.Active[slot] = true;
        RebuildArrays();
        return s_Data.Active[slot];
    }

    s_Data.Active[slot] = UploadSlot(slot, desc, true);
    if (!s_Data.Active[slot])
        UploadSlot(slot, desc, false);
    GenerateAllMips();
    return s_Data.Active[slot];
}

void TerrainMaterials::ClearSlot(uint32_t slot) {
    if (slot >= kMaxTerrainMaterialSlots || !s_Data.Active[slot])
        return;
    s_Data.Active[slot] = false;
    s_Data.Descs[slot] = {};
    if (AnySlotActive()) {
        UploadSlot(slot, s_Data.Descs[slot], false);
        GenerateAllMips();
    } else {
        RebuildArrays();
    }
}

bool TerrainMaterials::IsSlotActive(uint32_t slot) {
    return slot < kMaxTerrainMaterialSlots && s_Data.Active[slot];
}

const TerrainMaterialDesc& TerrainMaterials::GetSlotDesc(uint32_t slot) {
    return s_Data.Descs[std::min(slot, kMaxTerrainMaterialSlots - 1)];
}

void TerrainMaterials::SetSlotParams(uint32_t slot, const TerrainMaterialDesc& params) {
    if (slot >= kMaxTerrainMaterialSlots)
        return;
    TerrainMaterialDesc& desc = s_Data.Descs[slot];
    desc.Tint = params.Tint;
    desc.Roughness = params.Roughness;
    desc.Metallic = params.Metallic;
    desc.TextureScale = params.TextureScale;
    desc.NormalStrength = params.NormalStrength;
}

void TerrainMaterials::SetResolution(uint32_t resolution) {
    resolution = std::clamp(resolution, 1u, 8192u);
    if (resolution == s_Data.Resolution)
        return;
    s_Data.Resolution = resolution;
    if (AnySlotActive())
        RebuildArrays();
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
    if (!s_Data.AlbedoArray)
        return;
    s_Data.AlbedoArray->Bind(kAlbedoUnit);
    s_Data.NormalArray->Bind(kNormalUnit);
    s_Data.RoughnessArray->Bind(kRoughnessUnit);
    glActiveTexture(GL_TEXTURE0); // every other texture user assumes unit 0 is active
}

void TerrainMaterials::UploadUniforms(Shader* shader) {
    shader->SetInt("u_TerrainAlbedo", (int)kAlbedoUnit);
    shader->SetInt("u_TerrainNormal", (int)kNormalUnit);
    shader->SetInt("u_TerrainRoughness", (int)kRoughnessUnit);
    shader->SetFloat("u_TriplanarSharpness", s_Data.TriplanarSharpness);
    shader->SetFloat("u_TriplanarFaceNormal", s_Data.TriplanarFaceNormal);

    const auto& names = GetSlotUniformNames();
    for (uint32_t i = 0; i < kMaxTerrainMaterialSlots; i++) {
        const TerrainMaterialDesc& desc = s_Data.Descs[i];
        shader->SetVec3(names[i].Tint, desc.Tint);
        shader->SetFloat(names[i].Roughness, desc.Roughness);
        shader->SetFloat(names[i].Metallic, desc.Metallic);
        shader->SetFloat(names[i].Scale, desc.TextureScale);
        shader->SetFloat(names[i].NormalStrength, desc.NormalStrength);
        shader->SetInt(names[i].Active, s_Data.Active[i] ? 1 : 0);
    }
}

} // namespace Wankel
