#pragma once

#include "Wankel/Core/Base.h"
#include "Wankel/Particles/Particle.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <map>
#include <utility>
#include <vector>

namespace Wankel {

class Camera;
class ParticleRenderer;
struct ParticleEffect;
struct ParticleLayer;

// A reusable CPU particle simulation backed by a fixed pre-allocated pool. Not tied to the ECS - one
// ParticleSystem is shared by every ParticleEmitter in a scene (see ParticleEmitterSystem), so particles
// never become entities. Live particles point at their effect layer through a small registry of
// LayerSlots, which keeps the effect alive while any of its particles are.
class ParticleSystem {
public:
    // One (effect, layer index) in use by live particles. Effect stays referenced until the last of them dies.
    struct LayerSlot {
        Ref<ParticleEffect> Effect;
        uint32_t LayerIndex = 0;
        uint32_t LiveParticles = 0;

        // Null if the effect was edited so this layer no longer exists.
        const ParticleLayer* Layer() const;
    };

    struct Stats {
        uint32_t Alive = 0;
        uint32_t Capacity = 0;
        uint32_t PeakAlive = 0;
        uint64_t Dropped = 0; // particles that couldn't spawn because the pool was full
        uint32_t ActiveLayers = 0;
        uint32_t AlphaParticles = 0; // from the last Render
        uint32_t AdditiveParticles = 0;
        uint32_t DrawCalls = 0;
    };

    explicit ParticleSystem(uint32_t maxParticles = 4096);
    ~ParticleSystem();

    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;

    // One-shot: fires every enabled layer's BurstCount * times particles (after each layer's StartDelay)
    // from `origin` along unit `direction`. `velocity` is the source's own velocity, scaled per layer by
    // Emission.InheritVelocity. For impacts/tracers that have no emitter entity.
    void Play(const Ref<ParticleEffect>& effect, const glm::vec3& origin, const glm::vec3& direction,
              const glm::vec3& velocity = glm::vec3(0.0f), uint32_t times = 1);

    // Spawns `count` particles of one layer right now. Used by Play and ParticleEmitterSystem's continuous
    // emission; fewer are spawned (and counted as dropped) if the pool fills.
    void Emit(const Ref<ParticleEffect>& effect, uint32_t layerIndex, const glm::vec3& origin,
              const glm::vec3& direction, const glm::vec3& velocity, uint32_t count);

    // Integrates every live particle, fires due delayed bursts, and recycles expired particles.
    void Simulate(float dt);

    // Draws the live particles. Call between Renderer::BeginScene/EndScene.
    void Render(const Camera& camera);

    void Clear();

    uint32_t AliveCount() const { return m_AliveCount; }
    uint32_t Capacity() const { return (uint32_t)m_Pool.size(); }
    const Stats& GetStats() const { return m_Stats; }

private:
    struct PendingBurst {
        Ref<ParticleEffect> Effect;
        uint32_t LayerIndex = 0;
        glm::vec3 Origin {0.0f}, Direction {0.0f}, Velocity {0.0f};
        uint32_t Count = 0;
        float Delay = 0.0f;
    };

    uint16_t AcquireSlot(const Ref<ParticleEffect>& effect, uint32_t layerIndex);
    void ReleaseParticle(uint16_t slot);

    std::vector<Particle> m_Pool; // sized once in the ctor; live particles are the prefix [0, m_AliveCount)
    uint32_t m_AliveCount = 0;

    std::vector<LayerSlot> m_Slots;
    std::vector<uint16_t> m_FreeSlots;
    std::map<std::pair<const ParticleEffect*, uint32_t>, uint16_t> m_SlotLookup;

    std::vector<PendingBurst> m_Pending;
    float m_Time = 0.0f;
    float m_LastDropWarningTime = -1e9f;
    Stats m_Stats;

    Scope<ParticleRenderer> m_Renderer;
};

} // namespace Wankel
