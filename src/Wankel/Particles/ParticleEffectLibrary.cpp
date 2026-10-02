#include "wkpch.h"
#include "Wankel/Particles/ParticleEffectLibrary.h"

#include "Wankel/Math/Math.h"

namespace Wankel::ParticleEffects {

namespace {

Ref<ParticleEffect> SingleLayer(const char* name, const ParticleLayer& layer) {
    auto effect = CreateRef<ParticleEffect>();
    effect->Name = name;
    effect->Layers.push_back(layer);
    effect->Layers.back().Name = name;
    effect->Validate();
    return effect;
}

} // namespace

Ref<ParticleEffect> Smoke() {
    ParticleLayer l;
    l.Emission.Rate = 24.0f;
    l.Emission.BurstCount = 12;
    l.Emission.Shape = EmitShape::Cone;
    l.Emission.ConeAngleDegrees = 18.0f;
    l.Emission.LifetimeMin = 1.6f;
    l.Emission.LifetimeMax = 2.6f;
    l.Emission.SpeedMin = 0.6f;
    l.Emission.SpeedMax = 1.2f;
    l.Motion.Gravity = {0.0f, 0.35f, 0.0f}; // gentle buoyant rise
    l.Motion.Drag = 0.9f;
    l.Appearance.SizeMin = 0.25f;
    l.Appearance.SizeMax = 0.45f;
    l.Appearance.SizeOverLife = ParticleCurve::Linear(1.0f, 5.0f);
    l.Appearance.StartRotationJitter = Math::PI;
    l.Appearance.AngularVelocityMin = -0.8f;
    l.Appearance.AngularVelocityMax = 0.8f;
    l.Appearance.ColorOverLife = ParticleGradient::Linear({0.32f, 0.32f, 0.34f, 0.55f}, {0.18f, 0.18f, 0.20f, 0.0f});
    l.Material.Blend = ParticleBlend::Alpha;
    return SingleLayer("Smoke", l);
}

Ref<ParticleEffect> Sparks() {
    ParticleLayer l;
    l.Emission.BurstCount = 24;
    l.Emission.Shape = EmitShape::Cone;
    l.Emission.ConeAngleDegrees = 42.0f;
    l.Emission.LifetimeMin = 0.25f;
    l.Emission.LifetimeMax = 0.6f;
    l.Emission.SpeedMin = 6.0f;
    l.Emission.SpeedMax = 12.0f;
    l.Motion.Gravity = {0.0f, -14.0f, 0.0f};
    l.Motion.Drag = 1.2f;
    l.Appearance.SizeMin = 0.03f;
    l.Appearance.SizeMax = 0.06f;
    l.Appearance.SizeOverLife = ParticleCurve::Linear(1.0f, 0.25f);
    l.Appearance.ColorOverLife = ParticleGradient::Linear({1.0f, 0.85f, 0.45f, 1.0f}, {0.9f, 0.25f, 0.05f, 0.0f});
    l.Appearance.Orientation = ParticleOrientation::VelocityStretched;
    l.Appearance.Aspect = 2.0f;
    l.Appearance.StretchFactor = 0.02f;
    l.Material.Blend = ParticleBlend::Additive;
    return SingleLayer("Sparks", l);
}

Ref<ParticleEffect> Blood() {
    ParticleLayer l;
    l.Emission.BurstCount = 20;
    l.Emission.Shape = EmitShape::Sphere;
    l.Emission.LifetimeMin = 0.4f;
    l.Emission.LifetimeMax = 0.9f;
    l.Emission.SpeedMin = 1.5f;
    l.Emission.SpeedMax = 4.5f;
    l.Motion.Gravity = {0.0f, -9.8f, 0.0f};
    l.Motion.Drag = 0.4f;
    l.Appearance.SizeMin = 0.06f;
    l.Appearance.SizeMax = 0.14f;
    l.Appearance.SizeOverLife = ParticleCurve::Linear(1.0f, 0.3f);
    l.Appearance.ColorOverLife = ParticleGradient::Linear({0.5f, 0.02f, 0.02f, 1.0f}, {0.22f, 0.0f, 0.0f, 0.0f});
    l.Material.Blend = ParticleBlend::Alpha;
    return SingleLayer("Blood", l);
}

Ref<ParticleEffect> MuzzleFlash() {
    ParticleLayer l;
    l.Emission.BurstCount = 16; // ParticleEmitter::Burst() once per shot
    l.Emission.Shape = EmitShape::Cone;
    l.Emission.ConeAngleDegrees = 16.0f;
    l.Emission.LifetimeMin = 0.03f;
    l.Emission.LifetimeMax = 0.09f;
    l.Emission.SpeedMin = 4.0f;
    l.Emission.SpeedMax = 9.0f;
    l.Motion.Drag = 2.0f;
    l.Appearance.SizeMin = 0.10f;
    l.Appearance.SizeMax = 0.18f;
    l.Appearance.SizeOverLife = ParticleCurve::Linear(1.0f, 0.15f);
    l.Appearance.ColorOverLife = ParticleGradient::Linear({1.0f, 0.9f, 0.6f, 1.0f}, {1.0f, 0.45f, 0.1f, 0.0f});
    l.Material.Blend = ParticleBlend::Additive;
    return SingleLayer("MuzzleFlash", l);
}

} // namespace Wankel::ParticleEffects
