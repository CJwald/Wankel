#pragma once

#include "DebugDraw.h"
#include "Wankel/Core/Base.h"

#include <glm/glm.hpp>
#include <string>

namespace Wankel {

class Camera;
class Shader;
class Mesh;
class Font;
class OcclusionQuery;

struct FogSettings {
    glm::vec3 Color = {0.12f, 0.1f, 0.2f};

    float Density = 0.01f;

    // Noise fog
    bool NoiseEnabled = true;
    float NoiseScale = 0.05f;
    float NoiseStrength = 0.75f;
    int NoiseOctaves = 4;
    glm::vec3 WindDir = {1.0f, 0.0f, 0.0f};
    float WindSpeed = 0.2f;
};

struct LightSettings {
    // Direction the light travels (points FROM the light TOWARD the scene),
    // e.g. a sun low in the sky to the -X/-Z side.
    glm::vec3 Direction = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f));
    glm::vec3 Color = {1.0f, 0.98f, 0.92f};

    float Ambient = 0.25f;
    float Specular = 0.35f;
};

// GPU-ready snapshot of one point light: world position (read from the owning
// entity's Transform) plus the PointLight component's own fields. Built fresh
// each frame by whoever collects <Transform, PointLight> entities (Renderer
// has no ECS knowledge, same separation as Material vs MeshRenderer).
struct PointLightGPU {
    glm::vec3 Position {0.0f};
    glm::vec3 Color {1.0f};
    float Intensity = 1.0f;
    float Radius = 10.0f;
};

static constexpr size_t kMaxPointLights = 8;

// Solid-color, non-textured PBR material (metallic-roughness workflow).
// No UVs/textures in this pass - see Documents/TODO.md for the deferred
// texture-mapped-materials follow-up.
struct Material {
    glm::vec3 Albedo {0.8f, 0.8f, 0.8f};   // base color; also the F0 basis for metals
    float Roughness = 0.5f;                // 0 = mirror-smooth, 1 = fully rough
    float Metallic = 0.0f;                 // 0 = dielectric, 1 = metal
    glm::vec3 Emissive {0.0f, 0.0f, 0.0f}; // added post-lighting; default off, costs nothing unused

    // Exact equality (not approximate) - used by Renderer::Submit to skip re-uploading material
    // uniforms when consecutive draws share the identical value (e.g. every voxel chunk).
    bool operator==(const Material& other) const {
        return Albedo == other.Albedo && Roughness == other.Roughness && Metallic == other.Metallic &&
               Emissive == other.Emissive;
    }
};

class Renderer {
public:
    static void Init();
    static void Shutdown();

    static void BeginScene(const Camera& camera);
    static void EndScene();

    // Opaque Mesh Pass
    static void Submit(const glm::mat4& transform, const Mesh& mesh, Shader* shader, const Material& material);

    // Draws the same mesh once per entry in instanceOffsets (each a world-space translation added to
    // the vertex position after `transform`, via the aInstanceOffset vertex attribute at location 3 -
    // see cube.vert) in a single glDrawElementsInstanced call, instead of one Submit() per offset.
    static void SubmitInstanced(const glm::mat4& transform, const Mesh& mesh, Shader* shader, const Material& material,
                                const std::vector<glm::vec3>& instanceOffsets);

    // Occlusion culling - see OcclusionQuery.h. Typical use: BeginOcclusionQuery/EndOcclusionQuery
    // around a cheap (or real, color-write-disabled via SetColorWrite) proxy draw, then in a *later
    // frame* (never the same one - see OcclusionQuery::HasIssued()) BeginConditionalRender/
    // EndConditionalRender around the real draw so the GPU itself skips it if the query found nothing
    // visible - no CPU readback/stall either way.
    static void SetColorWrite(bool enabled);
    static void BeginOcclusionQuery(const OcclusionQuery& query);
    static void EndOcclusionQuery();
    static void BeginConditionalRender(const OcclusionQuery& query);
    static void EndConditionalRender();

    // Debug Pass
    static void SubmitDebugLines(const std::vector<DebugLine>& lines);

    // Gameplay Pass - unlike SubmitDebugLines, always drawn regardless of Renderer::DebugEnabled.
    // For player-facing wireframes (e.g. a voxel targeting highlight) that must be visible in
    // normal play, not just when the debug overlay is toggled on.
    static void SubmitGameplayLines(const std::vector<DebugLine>& lines);

    // Screen-space line overlay (pixels, Y-down, origin top-left; DebugLine.P0/P1.z is ignored) -
    // draws immediately, independent of BeginScene/EndScene's 3D camera, same as SubmitText. For
    // simple UI wireframes (e.g. a center-screen crosshair) where a font glyph isn't the right tool.
    static void SubmitScreenLines(const std::vector<DebugLine>& lines, uint32_t screenWidth, uint32_t screenHeight);

    // Screen-space text overlay (pixels, Y-down, origin top-left) - draws
    // immediately, independent of BeginScene/EndScene's 3D camera. Call
    // after EndScene(), same spot the ImGui pass runs.
    static void SubmitText(const std::string& text, const Ref<Font>& font, const glm::vec2& screenPos,
                           uint32_t screenWidth, uint32_t screenHeight, const glm::vec3& color = {1.0f, 1.0f, 1.0f});

    // Screen-space filled rectangle (pixels, Y-down, origin top-left) - same conventions/call timing as
    // SubmitText above. Solid color via `color`/`alpha` (blending is enabled globally in Init(), so
    // alpha < 1 works with no extra state) - for in-game UI panels/buttons that aren't ImGui debug
    // tooling (see MechtrixLayer's Backpack/Pause Menu).
    static void SubmitScreenQuad(const glm::vec2& min, const glm::vec2& max, const glm::vec3& color, float alpha,
                                 uint32_t screenWidth, uint32_t screenHeight);

    // Transparent Mesh Pass Eventually?
    // static void SubmitTransparent()...

    static void Draw(const Mesh& mesh);
    static void Clear();
    static void OnWindowResize(uint32_t width, uint32_t height);
    static void SetFog(const FogSettings& fog);
    static void SetLight(const LightSettings& light);
    static void SetPointLights(const std::vector<PointLightGPU>& lights);

    static bool DebugEnabled; // Global toggle
};

} // namespace Wankel
