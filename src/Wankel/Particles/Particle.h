#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace Wankel {

// One live particle in a ParticleSystem's pool. Fully self-contained: every value the simulation and
// the renderer need is baked in at spawn (sampled from a ParticleEffect), so ticking a particle
// never touches the effect it came from - the effect can be changed or destroyed freely afterward.
struct Particle {
    glm::vec3 Position {0.0f};
    glm::vec3 Velocity {0.0f};
    glm::vec3 Acceleration {0.0f};

    float Age = 0.0f;
    float Lifetime = 1.0f;

    float StartSize = 1.0f;
    float EndSize = 1.0f;

    float Rotation = 0.0f;        // billboard roll, radians
    float AngularVelocity = 0.0f; // radians/sec

    glm::vec4 StartColor {1.0f}; // rgba, alpha included
    glm::vec4 EndColor {1.0f};

    float Drag = 0.0f; // fraction of speed shed per second

    // Mirrors ParticleBlend - stored per particle (as the raw value, not the enum, to keep this a
    // trivially-copyable POD) so one pool can hold effects of both blend kinds and the renderer can
    // bin them without a back-reference to the effect.
    uint8_t Blend = 0;
};

} // namespace Wankel
