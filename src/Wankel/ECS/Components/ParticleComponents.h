#pragma once

#include "Wankel/Core/Base.h"
#include "Wankel/Particles/ParticleEffect.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace Wankel {

// Attach to any entity with a Transform to make it emit a particle effect. The particles live in a
// shared ParticleSystem pool (driven by ParticleEmitterSystem), never as entities. One effect per
// emitter, but an effect can hold several layers (flash + smoke + sparks).
struct ParticleEmitter {
    Ref<ParticleEffect> Effect; // shared - usually from ParticleLibrary::Load, so edits apply live

    bool Enabled = true; // gates continuous emission (each layer's Emission.Rate); bursts fire regardless

    glm::vec3 LocalOffset {0.0f};                 // emission point, entity space
    glm::vec3 LocalDirection {0.0f, 0.0f, -1.0f}; // emit axis, entity space (-Z matches Camera::GetForward)

    // Runtime state - owned by ParticleEmitterSystem / gameplay, not authored tuning.
    std::vector<float> SpawnAccumulators; // per layer - fractional carry so a low Rate still averages out
    uint32_t PendingBursts = 0;           // gameplay adds via Burst(); the system drains it to 0 each tick
    glm::vec3 PreviousOrigin {0.0f};      // for the emitter's own velocity (layers' InheritVelocity)
    bool HasPreviousOrigin = false;

    // Queues `times` bursts of every layer's Emission.BurstCount for the next system tick - e.g. one
    // gunshot's muzzle flash. Additive across calls within a frame.
    void Burst(uint32_t times = 1) { PendingBursts += times; }
};

} // namespace Wankel
