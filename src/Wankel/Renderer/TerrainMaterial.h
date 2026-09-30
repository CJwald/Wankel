#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <string>

namespace Wankel {

class Shader;

// Fixed by the vertex format: one normalized byte per slot, split across two ubyte4 attributes.
constexpr uint32_t kMaxTerrainMaterialSlots = 8;

// One bit per texture channel - which maps a slot actually loaded, so the shader samples only those.
enum TerrainMap : uint16_t {
    TerrainMap_BaseColor = 1u << 0, // sRGB color data
    TerrainMap_Normal = 1u << 1,    // linear, OpenGL (+Y) tangent-space convention
    TerrainMap_Roughness = 1u << 2, // linear scalar
    TerrainMap_Metallic = 1u << 3,  // linear scalar
    TerrainMap_Height = 1u << 4,    // linear scalar, 0.5 = surface level (visual parallax only, no displacement)
    TerrainMap_AO = 1u << 5,        // linear scalar
    TerrainMap_Emissive = 1u << 6,  // sRGB color data
    TerrainMap_Opacity = 1u << 7,   // linear scalar
    TerrainMap_Mask = 1u << 8,      // linear scalar - generic procedural control mask, exposed to the shader only
};

// Triplanar-mapped terrain material. Every map is optional - an empty path means that channel uses its
// param below instead (never sampled). BaseColorTint and EmissiveStrength also multiply their maps.
struct TerrainMaterialDesc {
    std::string BaseColorPath;
    std::string NormalPath;
    std::string RoughnessPath;
    std::string MetallicPath;
    std::string HeightPath;
    std::string AOPath;
    std::string EmissivePath;
    std::string OpacityPath;
    std::string MaskPath;

    glm::vec3 BaseColorTint {1.0f}; // the base color itself when there's no base color map
    float Roughness = 1.0f;
    float Metallic = 0.0f;
    glm::vec3 EmissiveColor {0.0f};
    float EmissiveStrength = 1.0f;
    float Opacity = 1.0f;
    float TextureScale = 0.25f; // texture repeats per world unit
    float NormalStrength = 1.0f;
    float HeightScale = 0.0f; // parallax depth in world units; 0 = off even with a height map

    // Every `<dir>/<name>_<channel>.png` that exists (channel = basecolor, normal, roughness, metallic,
    // height, ao, emissive, opacity, mask) - missing ones are simply left unset.
    static TerrainMaterialDesc FromDirectory(const std::string& dir, const std::string& name);
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
// WankelShaders/terrain_material.glsl. All slots share one resolution; each texture array holds only as
// many layers as the slots actually using it need.
class TerrainMaterials {
public:
    static void Init();
    static void Shutdown();

    // Loads the slot's maps at the current resolution; returns false (slot left inactive) on failure.
    static bool SetSlot(uint32_t slot, const TerrainMaterialDesc& desc);
    static void ClearSlot(uint32_t slot);
    static bool IsSlotActive(uint32_t slot);
    static uint32_t GetSlotMaps(uint32_t slot); // TerrainMap bits of the maps that loaded

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

    // Texture units 1-5; bound on every terrain draw, even with no active slots, so the shader's
    // sampler2DArray uniforms never alias unit 0's sampler2D (a GL draw-time error).
    static void BindTextures();
    // Sampler bindings + slot params; a no-op on shaders that don't include terrain_material.glsl.
    static void UploadUniforms(Shader* shader);
};

} // namespace Wankel
