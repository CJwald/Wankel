#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace Wankel {

// One live particle in a ParticleSystem's pool - motion state plus the few per-particle random values
// drawn at spawn. Appearance (size/color curves, texture, flipbook) is read live from the particle's
// layer via LayerSlot, so editing an effect changes particles already in flight. Trivially copyable,
// so the pool's swap-remove stays a plain memberwise copy.
struct Particle {
    glm::vec3 Position {0.0f};
    glm::vec3 Velocity {0.0f};

    float Age = 0.0f;
    float Lifetime = 1.0f;

    float Size = 1.0f;            // base size, multiplied by the layer's SizeOverLife
    float Rotation = 0.0f;        // billboard roll, radians
    float AngularVelocity = 0.0f; // radians/sec
    float Seed = 0.0f;            // random [0,1) - noise offset, random flipbook start frame

    uint16_t LayerSlot = 0; // index into ParticleSystem's layer registry
};

} // namespace Wankel
