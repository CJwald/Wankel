#include "wkpch.h"
#include "Wankel/Particles/ParticleEffect.h"

#include <algorithm>

namespace Wankel {

namespace {

void OrderRange(float& min, float& max, float floor) {
    min = std::max(min, floor);
    max = std::max(max, floor);
    if (min > max)
        std::swap(min, max);
}

} // namespace

void ParticleLayer::Validate() {
    ParticleEmission& em = Emission;
    em.Rate = std::max(em.Rate, 0.0f);
    em.StartDelay = std::max(em.StartDelay, 0.0f);
    em.ConeAngleDegrees = std::clamp(em.ConeAngleDegrees, 0.0f, 180.0f);
    em.BoxHalfExtents = glm::max(em.BoxHalfExtents, glm::vec3(0.0f));
    OrderRange(em.LifetimeMin, em.LifetimeMax, 0.001f); // a zero lifetime would divide by zero in t = age/life
    OrderRange(em.SpeedMin, em.SpeedMax, 0.0f);

    Motion.Drag = std::max(Motion.Drag, 0.0f);
    Motion.TurbulenceStrength = std::max(Motion.TurbulenceStrength, 0.0f);

    OrderRange(Appearance.SizeMin, Appearance.SizeMax, 0.0f);
    if (Appearance.AngularVelocityMin > Appearance.AngularVelocityMax)
        std::swap(Appearance.AngularVelocityMin, Appearance.AngularVelocityMax);
    Appearance.Aspect = std::max(Appearance.Aspect, 0.01f);
    Appearance.StretchFactor = std::max(Appearance.StretchFactor, 0.0f);
    Appearance.SizeOverLife.Sort();
    Appearance.AlphaOverLife.Sort();
    Appearance.ColorOverLife.Sort();

    ParticleFlipbook& fb = Material.Flipbook;
    fb.Columns = std::max(fb.Columns, 1u);
    fb.Rows = std::max(fb.Rows, 1u);
    fb.FrameCount = std::clamp(fb.FrameCount, 1u, fb.Columns * fb.Rows);
    fb.FramesPerSecond = std::max(fb.FramesPerSecond, 0.0f);
    Material.EmissiveStrength = std::max(Material.EmissiveStrength, 0.0f);
}

} // namespace Wankel
