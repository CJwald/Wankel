#pragma once

#include "Wankel/Core/Base.h"
#include "Wankel/Particles/Particle.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace Wankel {

class Camera;
class ParticleRenderer;
struct ParticleEffect;

// A reusable CPU particle simulation backed by a fixed pre-allocated pool. Not tied to the ECS - one
// ParticleSystem is shared by every ParticleEmitter in a scene (see ParticleEmitterSystem), so
// particles never become entities. No GPU simulation, no collision - deliberately minimal.
class ParticleSystem {
public:
    explicit ParticleSystem(uint32_t maxParticles = 4096);
    ~ParticleSystem();

    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;

    // Emits up to `count` particles from `origin`. `direction` is the unit emit axis; `effect.Shape`
    // decides how each particle's actual direction derives from it. Emits fewer, with a warning, if
    // the pool fills.
    void Spawn(const ParticleEffect& effect, const glm::vec3& origin, const glm::vec3& direction, uint32_t count);

    // Integrates every live particle and recycles any that have outlived their lifetime.
    void Simulate(float dt);

    // Draws the live particles as camera-facing billboards. Call between Renderer::BeginScene/EndScene.
    void Render(const Camera& camera);

    uint32_t AliveCount() const { return m_AliveCount; }
    uint32_t Capacity() const { return (uint32_t)m_Pool.size(); }
    void Clear() { m_AliveCount = 0; }

private:
    std::vector<Particle> m_Pool; // sized once in the ctor; live particles are the prefix [0, m_AliveCount)
    uint32_t m_AliveCount = 0;

    Scope<ParticleRenderer> m_Renderer;
};

} // namespace Wankel
