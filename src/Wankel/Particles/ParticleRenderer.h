#pragma once

#include "Wankel/Core/Base.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace Wankel {

class Camera;
class Shader;
class Texture;
class VertexArray;
class VertexBuffer;
class IndexBuffer;
struct Particle;

// Draws a ParticleSystem's live particles as camera-facing textured billboards, one
// glDrawElementsInstanced call per blend mode. Owns its GL objects - a static unit quad plus a
// per-instance buffer refilled every frame via glBufferSubData, the same "allocate once, sub-data
// per use" pattern as Renderer's debug/text batches and ChunkGeometryPool. Construct only after the
// GL context exists.
class ParticleRenderer {
public:
    explicit ParticleRenderer(uint32_t maxParticles);
    ~ParticleRenderer();

    ParticleRenderer(const ParticleRenderer&) = delete;
    ParticleRenderer& operator=(const ParticleRenderer&) = delete;

    // Rebuilds the instance buffer from `particles[0, count)` and issues the draw. Saves/restores the
    // GL blend func and depth-mask state it changes, matching the engine's other passes.
    void Render(const Particle* particles, uint32_t count, const Camera& camera);

private:
    // One billboard's per-instance data - field order/offsets must match particle.vert's
    // location 2..5 attributes.
    struct InstanceData {
        glm::vec3 Center {0.0f};
        float Size = 0.0f;
        glm::vec4 Color {0.0f};
        float Rotation = 0.0f;
    };

    void DrawBin(const std::vector<InstanceData>& bin, uint32_t glSrcFactor, uint32_t glDstFactor);

    uint32_t m_MaxParticles = 0;

    Scope<VertexArray> m_VAO;
    Scope<VertexBuffer> m_QuadVBO;
    Scope<IndexBuffer> m_QuadIBO;
    uint32_t m_InstanceVBO = 0; // raw GL name - VertexArray/VertexBufferLayout can't express divisors

    Scope<Shader> m_Shader;
    Scope<Texture> m_Sprite;

    // Reused across frames so a steady particle count does no per-frame heap traffic.
    std::vector<InstanceData> m_AlphaInstances;
    std::vector<InstanceData> m_AdditiveInstances;
};

} // namespace Wankel
