#include "wkpch.h"
#include "Wankel/Particles/ParticleSystem.h"

#include "Wankel/Math/Random.h"
#include "Wankel/Particles/ParticleEffect.h"
#include "Wankel/Particles/ParticleRenderer.h"

#include <glm/glm.hpp>

namespace Wankel {

ParticleSystem::ParticleSystem(uint32_t maxParticles)
    : m_Pool(maxParticles), m_Renderer(CreateScope<ParticleRenderer>(maxParticles)) {}

ParticleSystem::~ParticleSystem() = default;

void ParticleSystem::Spawn(const ParticleEffect& effect, const glm::vec3& origin, const glm::vec3& direction,
                           uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (m_AliveCount >= m_Pool.size()) {
            WK_CORE_WARNING("ParticleSystem::Spawn - pool full ({0}), dropping {1} particle(s)", m_Pool.size(),
                            count - i);
            return;
        }

        Particle& p = m_Pool[m_AliveCount++];

        glm::vec3 dir = direction;
        if (effect.Shape == EmitShape::Cone)
            dir = Random::DirectionInCone(direction, effect.ConeAngleDegrees);
        else if (effect.Shape == EmitShape::Sphere)
            dir = Random::DirectionOnSphere();

        glm::vec3 spawnPos = origin;
        if (effect.Shape == EmitShape::Box) {
            spawnPos += glm::vec3(Random::Float(-effect.BoxHalfExtents.x, effect.BoxHalfExtents.x),
                                  Random::Float(-effect.BoxHalfExtents.y, effect.BoxHalfExtents.y),
                                  Random::Float(-effect.BoxHalfExtents.z, effect.BoxHalfExtents.z));
        }

        float speed = Random::Float(effect.SpeedMin, effect.SpeedMax);

        p.Position = spawnPos;
        p.Velocity = dir * speed + effect.InheritVelocity;
        p.Acceleration = effect.Gravity;
        p.Age = 0.0f;
        p.Lifetime = glm::max(0.0001f, Random::Float(effect.LifetimeMin, effect.LifetimeMax));
        p.StartSize = Random::Float(effect.StartSizeMin, effect.StartSizeMax);
        p.EndSize = effect.EndSize;
        p.Rotation = Random::Float(-effect.StartRotationJitter, effect.StartRotationJitter);
        p.AngularVelocity = Random::Float(effect.AngularVelocityMin, effect.AngularVelocityMax);
        p.StartColor = effect.StartColor;
        p.EndColor = effect.EndColor;
        p.Drag = effect.Drag;
        p.Blend = (uint8_t)effect.Blend;
    }
}

void ParticleSystem::Simulate(float dt) {
    for (uint32_t i = 0; i < m_AliveCount;) {
        Particle& p = m_Pool[i];
        p.Age += dt;
        if (p.Age >= p.Lifetime) {
            m_Pool[i] = m_Pool[--m_AliveCount]; // swap-remove; the moved-in particle reuses this slot
            continue;
        }
        p.Velocity += p.Acceleration * dt;
        p.Velocity *= glm::max(0.0f, 1.0f - p.Drag * dt);
        p.Position += p.Velocity * dt;
        p.Rotation += p.AngularVelocity * dt;
        i++;
    }
}

void ParticleSystem::Render(const Camera& camera) {
    m_Renderer->Render(m_Pool.data(), m_AliveCount, camera);
}

} // namespace Wankel
