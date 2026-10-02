#include "wkpch.h"
#include "Wankel/Particles/ParticleSystem.h"

#include "Wankel/Math/Noise.h"
#include "Wankel/Math/Random.h"
#include "Wankel/Particles/ParticleEffect.h"
#include "Wankel/Particles/ParticleRenderer.h"
#include "Wankel/Renderer/Renderer.h"

#include <glm/glm.hpp>

namespace Wankel {

const ParticleLayer* ParticleSystem::LayerSlot::Layer() const {
    return Effect && LayerIndex < Effect->Layers.size() ? &Effect->Layers[LayerIndex] : nullptr;
}

ParticleSystem::ParticleSystem(uint32_t maxParticles)
    : m_Pool(maxParticles), m_Renderer(CreateScope<ParticleRenderer>(maxParticles)) {
    m_Stats.Capacity = maxParticles;
}

ParticleSystem::~ParticleSystem() = default;

uint16_t ParticleSystem::AcquireSlot(const Ref<ParticleEffect>& effect, uint32_t layerIndex) {
    auto key = std::make_pair(effect.get(), layerIndex);
    auto it = m_SlotLookup.find(key);
    if (it != m_SlotLookup.end())
        return it->second;

    uint16_t slot;
    if (!m_FreeSlots.empty()) {
        slot = m_FreeSlots.back();
        m_FreeSlots.pop_back();
    } else {
        slot = (uint16_t)m_Slots.size();
        m_Slots.emplace_back();
    }
    m_Slots[slot] = {effect, layerIndex, 0};
    m_SlotLookup[key] = slot;
    return slot;
}

void ParticleSystem::ReleaseParticle(uint16_t slot) {
    LayerSlot& s = m_Slots[slot];
    if (--s.LiveParticles > 0)
        return;
    m_SlotLookup.erase(std::make_pair(s.Effect.get(), s.LayerIndex));
    s.Effect.reset();
    m_FreeSlots.push_back(slot);
}

void ParticleSystem::Play(const Ref<ParticleEffect>& effect, const glm::vec3& origin, const glm::vec3& direction,
                          const glm::vec3& velocity, uint32_t times) {
    if (!effect || times == 0)
        return;
    for (uint32_t i = 0; i < effect->Layers.size(); i++) {
        const ParticleLayer& layer = effect->Layers[i];
        uint32_t count = layer.Emission.BurstCount * times;
        if (!layer.Enabled || count == 0)
            continue;
        if (layer.Emission.StartDelay > 0.0f)
            m_Pending.push_back({effect, i, origin, direction, velocity, count, layer.Emission.StartDelay});
        else
            Emit(effect, i, origin, direction, velocity, count);
    }
}

void ParticleSystem::Emit(const Ref<ParticleEffect>& effect, uint32_t layerIndex, const glm::vec3& origin,
                          const glm::vec3& direction, const glm::vec3& velocity, uint32_t count) {
    if (!effect || layerIndex >= effect->Layers.size() || count == 0)
        return;
    const ParticleLayer& layer = effect->Layers[layerIndex];
    const ParticleEmission& em = layer.Emission;
    const ParticleAppearance& look = layer.Appearance;

    uint32_t room = (uint32_t)m_Pool.size() - m_AliveCount;
    if (count > room) {
        m_Stats.Dropped += count - room;
        if (m_Time - m_LastDropWarningTime > 1.0f) {
            WK_CORE_WARNING("ParticleSystem - pool full ({0}), dropping particles", m_Pool.size());
            m_LastDropWarningTime = m_Time;
        }
        count = room;
    }
    if (count == 0)
        return;

    glm::vec3 axis = glm::dot(direction, direction) > 1e-8f ? glm::normalize(direction) : glm::vec3(0, 0, -1);
    uint16_t slot = AcquireSlot(effect, layerIndex);
    m_Slots[slot].LiveParticles += count;

    for (uint32_t i = 0; i < count; i++) {
        Particle& p = m_Pool[m_AliveCount++];

        glm::vec3 dir = axis;
        if (em.Shape == EmitShape::Cone)
            dir = Random::DirectionInCone(axis, em.ConeAngleDegrees);
        else if (em.Shape == EmitShape::Sphere)
            dir = Random::DirectionOnSphere();

        glm::vec3 spawnPos = origin + em.LocalOffset;
        if (em.Shape == EmitShape::Box) {
            spawnPos += glm::vec3(Random::Float(-em.BoxHalfExtents.x, em.BoxHalfExtents.x),
                                  Random::Float(-em.BoxHalfExtents.y, em.BoxHalfExtents.y),
                                  Random::Float(-em.BoxHalfExtents.z, em.BoxHalfExtents.z));
        }

        p.Position = spawnPos;
        p.Velocity = dir * Random::Float(em.SpeedMin, em.SpeedMax) + velocity * em.InheritVelocity + em.AddedVelocity;
        p.Age = 0.0f;
        p.Lifetime = Random::Float(em.LifetimeMin, em.LifetimeMax);
        p.Size = Random::Float(look.SizeMin, look.SizeMax);
        p.Rotation = Random::Float(-look.StartRotationJitter, look.StartRotationJitter);
        p.AngularVelocity = Random::Float(look.AngularVelocityMin, look.AngularVelocityMax);
        p.Seed = Random::Float();
        p.LayerSlot = slot;
    }

    m_Stats.PeakAlive = glm::max(m_Stats.PeakAlive, m_AliveCount);
}

void ParticleSystem::Simulate(float dt) {
    m_Time += dt;

    // Delayed layers of earlier Play() calls.
    for (size_t i = 0; i < m_Pending.size();) {
        PendingBurst& b = m_Pending[i];
        b.Delay -= dt;
        if (b.Delay > 0.0f) {
            i++;
            continue;
        }
        Emit(b.Effect, b.LayerIndex, b.Origin, b.Direction, b.Velocity, b.Count);
        m_Pending[i] = std::move(m_Pending.back());
        m_Pending.pop_back();
    }

    for (uint32_t i = 0; i < m_AliveCount;) {
        Particle& p = m_Pool[i];
        const ParticleLayer* layer = m_Slots[p.LayerSlot].Layer();
        p.Age += dt;
        if (!layer || p.Age >= p.Lifetime) {
            ReleaseParticle(p.LayerSlot);
            m_Pool[i] = m_Pool[--m_AliveCount]; // swap-remove; the moved-in particle reuses this slot
            continue;
        }

        const ParticleMotion& motion = layer->Motion;
        glm::vec3 accel = motion.Gravity;
        if (motion.TurbulenceStrength > 0.0f) {
            // Three decorrelated noise lookups give a smoothly varying force direction per particle.
            glm::vec3 q = p.Position * motion.TurbulenceFrequency + glm::vec3(p.Seed * 37.0f) +
                          glm::vec3(0.0f, m_Time * motion.TurbulenceScroll, 0.0f);
            accel += motion.TurbulenceStrength * glm::vec3(Noise::PerlinNoise(q.x, q.y, q.z),
                                                           Noise::PerlinNoise(q.x + 31.4f, q.y, q.z + 17.2f),
                                                           Noise::PerlinNoise(q.x + 71.3f, q.y + 5.9f, q.z));
        }

        p.Velocity += accel * dt;
        p.Velocity *= glm::max(0.0f, 1.0f - motion.Drag * dt);
        p.Position += p.Velocity * dt;
        p.Rotation += p.AngularVelocity * dt;
        i++;
    }

    m_Stats.Alive = m_AliveCount;
    m_Stats.ActiveLayers = (uint32_t)m_SlotLookup.size();
}

void ParticleSystem::Render(const Camera& camera) {
    ParticleRenderer::FrameStats frame = m_Renderer->Render(m_Pool.data(), m_AliveCount, m_Slots, camera, m_Time);
    m_Stats.AlphaParticles = frame.AlphaParticles;
    m_Stats.AdditiveParticles = frame.AdditiveParticles;
    m_Stats.DrawCalls = frame.DrawCalls;
}

void ParticleSystem::SubmitDebugLines(const DebugOptions& options) const {
    if (!options.Velocity && !options.Centers)
        return;
    std::vector<DebugLine> lines;
    lines.reserve((size_t)m_AliveCount * (options.Centers ? 3 : 1));
    for (uint32_t i = 0; i < m_AliveCount; i++) {
        const Particle& p = m_Pool[i];
        if (options.Velocity)
            lines.push_back({p.Position, p.Position + p.Velocity * options.VelocityScale, {1.0f, 0.85f, 0.2f}});
        if (options.Centers) {
            constexpr float kCross = 0.02f;
            const glm::vec3 color(0.3f, 0.8f, 1.0f);
            lines.push_back({p.Position - glm::vec3(kCross, 0, 0), p.Position + glm::vec3(kCross, 0, 0), color});
            lines.push_back({p.Position - glm::vec3(0, kCross, 0), p.Position + glm::vec3(0, kCross, 0), color});
        }
    }
    Renderer::SubmitGameplayLines(lines);
}

void ParticleSystem::Clear() {
    m_AliveCount = 0;
    m_Pending.clear();
    m_Slots.clear();
    m_FreeSlots.clear();
    m_SlotLookup.clear();
    m_Stats.Alive = 0;
    m_Stats.ActiveLayers = 0;
}

} // namespace Wankel
