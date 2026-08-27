#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <string>

namespace Wankel {

enum class EmitShape : uint8_t {
    Point,  // every particle leaves along the emit axis
    Cone,   // random direction within ConeAngleDegrees of the emit axis
    Sphere, // random direction on the unit sphere (emit axis ignored)
    Box     // origin jittered within +/-BoxHalfExtents, direction along the emit axis
};

enum class ParticleBlend : uint8_t {
    Alpha,   // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA - smoke, blood
    Additive // GL_SRC_ALPHA, GL_ONE - sparks, muzzle flash, fire
};

// Pure configuration for one kind of particle effect - no runtime state (that lives on the Particle
// pool and the ParticleEmitter component). Same "one struct of tunable fields with in-class
// defaults" shape as MotionLink/MeshAnimation. Each Min/Max pair is sampled per particle at spawn;
// Min == Max just means "no variation".
struct ParticleEffect {
    // EMISSION
    float SpawnRate = 0.0f; // particles/sec for continuous emission; 0 = burst-only (ParticleEmitter::Burst)
    EmitShape Shape = EmitShape::Cone;
    float ConeAngleDegrees = 25.0f;  // half-angle, for EmitShape::Cone
    glm::vec3 BoxHalfExtents {0.0f}; // for EmitShape::Box

    // LIFETIME (seconds)
    float LifetimeMin = 1.0f;
    float LifetimeMax = 1.0f;

    // VELOCITY
    float SpeedMin = 1.0f;
    float SpeedMax = 2.0f;
    glm::vec3 InheritVelocity {0.0f}; // added to every particle's initial velocity (emitter drift / wind)

    // FORCES
    glm::vec3 Gravity {0.0f}; // constant acceleration
    float Drag = 0.0f;        // fraction of speed shed per second

    // SIZE (world units, full billboard width)
    float StartSizeMin = 0.2f;
    float StartSizeMax = 0.2f;
    float EndSize = 0.2f;

    // ROTATION
    float StartRotationJitter = 0.0f; // +/- this many radians of random initial roll
    float AngularVelocityMin = 0.0f;
    float AngularVelocityMax = 0.0f;

    // COLOR / ALPHA - linear lerp from Start to End over the particle's life
    glm::vec4 StartColor {1.0f};
    glm::vec4 EndColor {1.0f, 1.0f, 1.0f, 0.0f};

    // RENDER
    ParticleBlend Blend = ParticleBlend::Alpha;

    // Reserved for a future RGBA/atlas texture path - the renderer only has a built-in soft round
    // sprite for now and asserts these are left at their defaults rather than silently ignoring them.
    std::string TexturePath;
    glm::vec4 AtlasRect {0.0f, 0.0f, 1.0f, 1.0f}; // x, y, w, h in UV space
};

} // namespace Wankel
