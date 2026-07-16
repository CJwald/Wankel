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

// Solid-color, non-textured PBR material (metallic-roughness workflow).
// No UVs/textures in this pass - see Documents/TODO.md for the deferred
// texture-mapped-materials follow-up.
struct Material {
    glm::vec3 Albedo {0.8f, 0.8f, 0.8f}; // base color; also the F0 basis for metals
    float Roughness = 0.5f;              // 0 = mirror-smooth, 1 = fully rough
    float Metallic = 0.0f;               // 0 = dielectric, 1 = metal
    glm::vec3 Emissive {0.0f, 0.0f, 0.0f}; // added post-lighting; default off, costs nothing unused
};

class Renderer {
public:
    static void Init();
    static void Shutdown();

    static void BeginScene(const Camera& camera);
    static void EndScene();

    // Opaque Mesh Pass
    static void Submit(const glm::mat4& transform, const Mesh& mesh, Shader* shader, const Material& material);

    // Debug Pass
    static void SubmitDebugLines(const std::vector<DebugLine>& lines);

    // Screen-space text overlay (pixels, Y-down, origin top-left) - draws
    // immediately, independent of BeginScene/EndScene's 3D camera. Call
    // after EndScene(), same spot the ImGui pass runs.
    static void SubmitText(const std::string& text, const Ref<Font>& font, const glm::vec2& screenPos,
                           uint32_t screenWidth, uint32_t screenHeight,
                           const glm::vec3& color = {1.0f, 1.0f, 1.0f});

    // Transparent Mesh Pass Eventually?
    // static void SubmitTransparent()...

    static void Draw(const Mesh& mesh);
    static void Clear();
    static void OnWindowResize(uint32_t width, uint32_t height);
    static void SetFog(const FogSettings& fog);
    static void SetLight(const LightSettings& light);

    static bool DebugEnabled; // Global toggle
};

} // namespace Wankel
