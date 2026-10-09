#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace Wankel {

class Shader;

// Fixed by the vertex format: one normalized byte per slot, split across four ubyte4 attributes.
constexpr uint32_t kMaxTerrainMaterialSlots = 16;
constexpr uint32_t kTerrainMaterialWeightVec4s = kMaxTerrainMaterialSlots / 4; // vertex attributes per weight set

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

    // Alternate texture sets of the same material (map paths only), picked per world-space cell when
    // TextureVariationSettings::Variants is on. Expected to provide the same channels as the main maps.
    std::vector<TerrainMaterialDesc> Variants;

    // Every `<dir>/<name>_<channel>.png` that exists (channel = basecolor, normal, roughness, metallic,
    // height, ao, emissive, opacity, mask) - missing ones are simply left unset. Variants come from
    // `<dir>/<name>_v<N>_<channel>.png` (N = 1..kMaxTerrainVariants-1), each needing at least a base color.
    static TerrainMaterialDesc FromDirectory(const std::string& dir, const std::string& name);
};

// Texture repetition mitigation for every terrain slot. Everything is derived from world position (never
// chunk coordinates) and wraps at the world period (TerrainMaterials::SetWorldPeriod), so chunk borders,
// regeneration, streaming order and a tiled world's seam never change the result. Scales are world units.
struct TextureVariationSettings {
    bool Enabled = true; // master switch - off renders exactly as without this feature

    struct TransformSettings {
        bool Enabled = true;
        float Scale = 4.0f;      // cell size; each cell's texture gets its own rotation/mirror
        float EdgeBlend = 0.15f; // fraction of a cell blended with its neighbours at the edges (0 = hard)
        bool Rotation = true;
        bool Mirror = true;
        uint32_t Seed = 1;
    } Transform;

    struct VariantSettings {
        bool Enabled = true;
        int Count = 8; // variants to choose among, capped per slot by how many loaded
        float Scale = 8.0f;
        uint32_t Seed = 2;
    } Variants;

    struct MacroSettings {
        bool Enabled = true;
        float Scale = 50.0f;
        float Strength = 0.12f; // albedo *= 1 +- Strength
        float Contrast = 1.0f;
        uint32_t Seed = 3;
    } Macro;

    struct DetailSettings {
        bool Enabled = false;
        float Scale = 1.0f; // repeats per world unit - the slot's own texture, sampled again at this scale
        float Strength = 0.3f;
        bool BaseColor = true;
        bool Normal = true;
        bool Roughness = true;
    } Detail;

    struct MaterialSettings {
        bool Enabled = true;
        float Scale = 20.0f;
        float Albedo = 0.05f; // each multiplies its channel by 1 +- this
        float Roughness = 0.05f;
        float Normal = 0.15f;
        uint32_t Seed = 4;
    } Material;

    enum class DebugView : uint8_t { None, MacroPattern, VariationCells, TextureVariant, TransformId };
    DebugView View = DebugView::None;
};

constexpr uint32_t kMaxTerrainVariants = 8; // per slot, the main maps included

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
    // Samples sourceSlot's textures (no extra texture memory) with this slot's own scalar params, e.g. one
    // greyscale map tinted differently per slot. Texture paths in `params` are ignored. Clearing the
    // source clears every slot sharing it; re-setting it via SetSlot keeps them sharing its new maps.
    static bool SetSlotShared(uint32_t slot, uint32_t sourceSlot, const TerrainMaterialDesc& params);
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

    // Live-edited; feature on/off switches become TEXVAR_* shader defines (see SyncShaderDefines), the rest uniforms.
    static TextureVariationSettings& GetVariationSettings();
    // Publishes the variation on/off switches as global shader defines, so disabled features compile out entirely.
    static void SyncShaderDefines();
    // The tiling period of the world being drawn (0 on an axis = not tiled). Variation patterns wrap at it so a
    // tiled world has no seam; set it whenever the world's extent changes.
    static void SetWorldPeriod(const glm::vec3& period);
    static glm::vec3 GetWorldPeriod();
    // How many texture variants a slot has (1 = just its main maps).
    static uint32_t GetSlotVariantCount(uint32_t slot);

    // Texture units 1-5; bound on every terrain draw, even with no active slots, so the shader's
    // sampler2DArray uniforms never alias unit 0's sampler2D (a GL draw-time error).
    static void BindTextures();
    // Sampler bindings + slot params; a no-op on shaders that don't include terrain_material.glsl.
    static void UploadUniforms(Shader* shader);
};

} // namespace Wankel
