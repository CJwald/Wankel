#pragma once

#include "Wankel/Core/Base.h"
#include "Wankel/Particles/ParticleSystem.h"

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

// Draws a ParticleSystem's live particles as instanced quads. Each particle's size/color/flipbook frame
// is evaluated from its layer every frame; instances are batched into runs that share a layer (texture,
// orientation, blend), all uploaded in one buffer and drawn with base-instance offsets. Alpha particles
// are sorted back-to-front; additive ones are order-independent and just grouped. Construct only after
// the GL context exists.
class ParticleRenderer {
public:
    struct FrameStats {
        uint32_t AlphaParticles = 0;
        uint32_t AdditiveParticles = 0;
        uint32_t DrawCalls = 0;
    };

    explicit ParticleRenderer(uint32_t maxParticles);
    ~ParticleRenderer();

    ParticleRenderer(const ParticleRenderer&) = delete;
    ParticleRenderer& operator=(const ParticleRenderer&) = delete;

    // Saves/restores the GL blend func and depth-mask state it changes, matching the engine's other passes.
    // `time` drives distortion scrolling (seconds, any monotonic clock).
    FrameStats Render(const Particle* particles, uint32_t count, const std::vector<ParticleSystem::LayerSlot>& slots,
                      const Camera& camera, float time);

private:
    // One quad's per-instance data - field order/offsets must match particle.vert's location 2..8 attributes.
    struct InstanceData {
        glm::vec3 Center {0.0f};
        float Size = 0.0f;
        glm::vec3 Velocity {0.0f};
        float Rotation = 0.0f;
        glm::vec4 Color {0.0f};
        glm::vec4 UVRect {0.0f, 0.0f, 1.0f, 1.0f}; // u0, v0, du, dv
        float ErosionThreshold = 0.0f;             // layer's ErosionOverLife at this particle's age
        float Seed = 0.0f;                         // offsets the noise per particle
    };

    struct DrawItem {
        float SortKey = 0.0f; // squared camera distance (alpha) - additive items sort by slot only
        uint16_t Slot = 0;
        InstanceData Data;
    };

    // Consecutive instances sharing one layer slot - one draw call.
    struct Run {
        uint16_t Slot = 0;
        uint32_t First = 0;
        uint32_t Count = 0;
        bool Additive = false;
    };

    void BuildRuns(std::vector<DrawItem>& items, bool additive);
    void DrawRun(const Run& run, const std::vector<ParticleSystem::LayerSlot>& slots);

    uint32_t m_MaxParticles = 0;

    Scope<VertexArray> m_VAO;
    Scope<VertexBuffer> m_QuadVBO;
    Scope<IndexBuffer> m_QuadIBO;
    uint32_t m_InstanceVBO = 0; // raw GL name - VertexArray/VertexBufferLayout can't express divisors

    Scope<Shader> m_Shader;
    Scope<Texture> m_DefaultSprite; // built-in soft round sprite, used when a layer has no TexturePath
    Scope<Texture> m_Noise;         // tiling value noise for erosion/distortion

    // Reused across frames so a steady particle count does no per-frame heap traffic.
    std::vector<DrawItem> m_AlphaItems;
    std::vector<DrawItem> m_AdditiveItems;
    std::vector<InstanceData> m_Instances;
    std::vector<Run> m_Runs;
};

} // namespace Wankel
