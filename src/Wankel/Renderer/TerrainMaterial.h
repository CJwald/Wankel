#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <string>

namespace Wankel {

class Shader;
class TextureArray;

// Fixed by the vertex format: one normalized byte per slot, split across two ubyte4 attributes.
constexpr uint32_t kMaxTerrainMaterialSlots = 8;

// Triplanar-mapped terrain material. An empty path falls back to white albedo / flat normal / white roughness.
struct TerrainMaterialDesc {
    std::string AlbedoPath;
    std::string NormalPath; // OpenGL (+Y) tangent-space convention
    std::string RoughnessPath;
    glm::vec3 Tint {1.0f};
    float Roughness = 1.0f; // multiplies the roughness map
    float Metallic = 0.0f;
    float TextureScale = 0.25f; // texture repeats per world unit
    float NormalStrength = 1.0f;
};

// Per-vertex slot weights. A textured vertex bakes Color=0 plus a one-hot weight, so interpolated
// color + weighted texture samples reproduces the plain vertex-color blend exactly.
struct TerrainMaterialWeights {
    std::array<uint8_t, kMaxTerrainMaterialSlots> Slots {};

    static TerrainMaterialWeights OneHot(uint32_t slot) {
        TerrainMaterialWeights w;
        if (slot < kMaxTerrainMaterialSlots)
            w.Slots[slot] = 255;
        return w;
    }

    bool operator==(const TerrainMaterialWeights& other) const { return Slots == other.Slots; }
    bool operator!=(const TerrainMaterialWeights& other) const { return Slots != other.Slots; }
};

// Global set of textured terrain materials, sampled by terrain shaders that include
// WankelShaders/terrain_material.glsl. All slots share one resolution (one texture array per map type).
class TerrainMaterials {
public:
    static void Init();
    static void Shutdown();

    // Loads the slot's maps at the current resolution; returns false (slot left inactive) on failure.
    static bool SetSlot(uint32_t slot, const TerrainMaterialDesc& desc);
    static void ClearSlot(uint32_t slot);
    static bool IsSlotActive(uint32_t slot);

    // Scalar params only (no reload) - for live tuning. Texture paths are ignored.
    static const TerrainMaterialDesc& GetSlotDesc(uint32_t slot);
    static void SetSlotParams(uint32_t slot, const TerrainMaterialDesc& params);

    // Square texture size every map is resized to; changing it reloads every active slot from disk.
    static void SetResolution(uint32_t resolution);
    static uint32_t GetResolution();

    static void SetTriplanarSharpness(float sharpness);
    static float GetTriplanarSharpness();

    // 0 = project along the smooth vertex normal, 1 (default) = along the true triangle normal (no edge smearing).
    static void SetTriplanarFaceNormal(float blend);
    static float GetTriplanarFaceNormal();

    // Texture units 1-3; bound on every terrain draw, even with no active slots, so the shader's
    // sampler2DArray uniforms never alias unit 0's sampler2D (a GL draw-time error).
    static void BindTextures();
    // Sampler bindings + slot params; a no-op on shaders that don't include terrain_material.glsl.
    static void UploadUniforms(Shader* shader);

private:
    static void RebuildArrays();
};

} // namespace Wankel
