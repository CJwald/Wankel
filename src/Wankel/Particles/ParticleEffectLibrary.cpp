#include "wkpch.h"
#include "Wankel/Particles/ParticleEffectLibrary.h"

#include "Wankel/Math/Math.h"

namespace Wankel::ParticleEffects {

ParticleEffect Smoke() {
    ParticleEffect e;
    e.SpawnRate = 24.0f;
    e.Shape = EmitShape::Cone;
    e.ConeAngleDegrees = 18.0f;
    e.LifetimeMin = 1.6f;
    e.LifetimeMax = 2.6f;
    e.SpeedMin = 0.6f;
    e.SpeedMax = 1.2f;
    e.Gravity = {0.0f, 0.35f, 0.0f}; // gentle buoyant rise
    e.Drag = 0.9f;
    e.StartSizeMin = 0.25f;
    e.StartSizeMax = 0.45f;
    e.EndSize = 1.8f;
    e.StartRotationJitter = Math::PI;
    e.AngularVelocityMin = -0.8f;
    e.AngularVelocityMax = 0.8f;
    e.StartColor = {0.32f, 0.32f, 0.34f, 0.55f};
    e.EndColor = {0.18f, 0.18f, 0.20f, 0.0f};
    e.Blend = ParticleBlend::Alpha;
    return e;
}

ParticleEffect Sparks() {
    ParticleEffect e;
    e.SpawnRate = 0.0f; // burst-only
    e.Shape = EmitShape::Cone;
    e.ConeAngleDegrees = 42.0f;
    e.LifetimeMin = 0.25f;
    e.LifetimeMax = 0.6f;
    e.SpeedMin = 6.0f;
    e.SpeedMax = 12.0f;
    e.Gravity = {0.0f, -14.0f, 0.0f};
    e.Drag = 1.2f;
    e.StartSizeMin = 0.03f;
    e.StartSizeMax = 0.06f;
    e.EndSize = 0.01f;
    e.StartColor = {1.0f, 0.85f, 0.45f, 1.0f};
    e.EndColor = {0.9f, 0.25f, 0.05f, 0.0f};
    e.Blend = ParticleBlend::Additive;
    return e;
}

ParticleEffect Blood() {
    ParticleEffect e;
    e.SpawnRate = 0.0f; // burst-only
    e.Shape = EmitShape::Sphere;
    e.LifetimeMin = 0.4f;
    e.LifetimeMax = 0.9f;
    e.SpeedMin = 1.5f;
    e.SpeedMax = 4.5f;
    e.Gravity = {0.0f, -9.8f, 0.0f};
    e.Drag = 0.4f;
    e.StartSizeMin = 0.06f;
    e.StartSizeMax = 0.14f;
    e.EndSize = 0.03f;
    e.StartColor = {0.5f, 0.02f, 0.02f, 1.0f};
    e.EndColor = {0.22f, 0.0f, 0.0f, 0.0f};
    e.Blend = ParticleBlend::Alpha;
    return e;
}

ParticleEffect MuzzleFlash() {
    ParticleEffect e;
    e.SpawnRate = 0.0f; // burst-only - ParticleEmitter::Burst() once per shot
    e.Shape = EmitShape::Cone;
    e.ConeAngleDegrees = 16.0f;
    e.LifetimeMin = 0.03f;
    e.LifetimeMax = 0.09f;
    e.SpeedMin = 4.0f;
    e.SpeedMax = 9.0f;
    e.Drag = 2.0f;
    e.StartSizeMin = 0.10f;
    e.StartSizeMax = 0.18f;
    e.EndSize = 0.02f;
    e.StartColor = {1.0f, 0.9f, 0.6f, 1.0f};
    e.EndColor = {1.0f, 0.45f, 0.1f, 0.0f};
    e.Blend = ParticleBlend::Additive;
    return e;
}

} // namespace Wankel::ParticleEffects
