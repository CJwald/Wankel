#pragma once

#include "Wankel/Particles/ParticleEffect.h"

#include <glm/glm.hpp>

#include <cstdint>

namespace Wankel {

// Attach to any entity with a Transform to make it emit a particle effect. The particles live in a
// shared ParticleSystem pool (driven by ParticleEmitterSystem), never as entities. One effect per
// emitter - layered looks (e.g. flash + smoke) use one child entity per emitter.
struct ParticleEmitter {
    ParticleEffect Effect;

    bool Enabled = true; // gates continuous emission (Effect.SpawnRate); bursts fire regardless

    glm::vec3 LocalOffset {0.0f};                 // emission point, entity space
    glm::vec3 LocalDirection {0.0f, 0.0f, -1.0f}; // emit axis, entity space (-Z matches Camera::GetForward)

    // Runtime state - owned by ParticleEmitterSystem / gameplay, not authored tuning.
    float SpawnAccumulator = 0.0f; // fractional carry so a low SpawnRate still averages out correctly
    uint32_t PendingBurst = 0;     // gameplay adds via Burst(); the system drains it to 0 each tick

    // Queues a one-off puff of `count` particles for the next system tick - e.g. a gunshot's muzzle
    // flash. Additive across calls within a frame.
    void Burst(uint32_t count) { PendingBurst += count; }
};

} // namespace Wankel
