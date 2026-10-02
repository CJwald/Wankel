#pragma once

#include "Wankel/Particles/ParticleCurve.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

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

enum class ParticleOrientation : uint8_t {
    Billboard,        // camera-facing quad - smoke, fire, explosions
    VelocityAligned,  // long axis along the velocity, length = Size * Aspect - sparks, debris
    VelocityStretched // as VelocityAligned, plus speed * StretchFactor extra length - tracers, streaks
};

// How particles are born. Rate is continuous emission (needs a ParticleEmitter); BurstCount is what one
// ParticleEmitter::Burst() / ParticleSystem::Play() spawns.
struct ParticleEmission {
    float Rate = 0.0f;       // particles/sec while the emitter is Enabled; 0 = burst-only
    uint32_t BurstCount = 1; // particles per burst/play
    float StartDelay = 0.0f; // seconds after a burst/play before this layer's burst fires

    EmitShape Shape = EmitShape::Cone;
    float ConeAngleDegrees = 25.0f;  // half-angle, for EmitShape::Cone
    glm::vec3 BoxHalfExtents {0.0f}; // for EmitShape::Box
    glm::vec3 LocalOffset {0.0f};    // added to the emit origin, in world axes

    float LifetimeMin = 1.0f; // seconds
    float LifetimeMax = 1.0f;
    float SpeedMin = 1.0f; // along the emitted direction
    float SpeedMax = 2.0f;
    float InheritVelocity = 0.0f;   // fraction of the emitter/caller velocity added to each particle
    glm::vec3 AddedVelocity {0.0f}; // constant velocity added to every particle (drift / wind)
};

// How live particles move. Turbulence is a cheap deterministic noise force, not a fluid sim.
struct ParticleMotion {
    glm::vec3 Gravity {0.0f}; // constant acceleration
    float Drag = 0.0f;        // fraction of speed shed per second

    float TurbulenceStrength = 0.0f;  // acceleration magnitude; 0 = off
    float TurbulenceFrequency = 0.5f; // noise cycles per world unit
    float TurbulenceScroll = 0.5f;    // noise field drift speed
};

// Size/color/rotation over a particle's life. Curves are sampled with normalized age t in [0,1].
struct ParticleAppearance {
    float SizeMin = 0.2f; // world units, full quad width; one random value per particle
    float SizeMax = 0.2f;
    ParticleCurve SizeOverLife = ParticleCurve::Constant(1.0f); // multiplies the per-particle size

    ParticleGradient ColorOverLife = ParticleGradient::Linear({1, 1, 1, 1}, {1, 1, 1, 0});
    ParticleCurve AlphaOverLife = ParticleCurve::Constant(1.0f); // multiplies the gradient's alpha

    float StartRotationJitter = 0.0f; // +/- radians of random initial roll (billboards)
    float AngularVelocityMin = 0.0f;  // radians/sec
    float AngularVelocityMax = 0.0f;

    ParticleOrientation Orientation = ParticleOrientation::Billboard;
    float Aspect = 1.0f;        // length / width for the velocity-aligned modes
    float StretchFactor = 0.0f; // extra length per unit of speed (VelocityStretched)
};

// Texture-sheet animation over a sub-rect of the texture, left-to-right then top-to-bottom.
struct ParticleFlipbook {
    bool Enabled = false;
    uint32_t Columns = 1;
    uint32_t Rows = 1;
    uint32_t FrameCount = 1;
    float FramesPerSecond = 0.0f; // 0 = play the frames once over the particle's whole life
    bool Loop = false;            // with FramesPerSecond > 0: wrap instead of holding the last frame
    bool RandomStartFrame = false;
};

struct ParticleMaterial {
    std::string TexturePath;                      // empty = the built-in soft round sprite
    glm::vec4 AtlasRect {0.0f, 0.0f, 1.0f, 1.0f}; // x, y, w, h in UV space, y measured from the image top
    ParticleFlipbook Flipbook;
    ParticleBlend Blend = ParticleBlend::Alpha;
    float EmissiveStrength = 1.0f; // multiplies rgb - above 1 for bright additive effects

    // Optional shader features - each only costs anything for layers that enable it.
    bool SoftParticles = false; // fade out where the quad intersects scene geometry
    float SoftDistance = 0.5f;  // world units of depth over which that fade happens

    bool Erosion = false; // noise-threshold dissolve: solid -> irregular breakup -> gone
    ParticleCurve ErosionOverLife = ParticleCurve::Linear(0.0f, 1.0f); // threshold: 0 = intact, 1 = fully eroded
    float ErosionSoftness = 0.1f;                                      // width of the dissolving edge

    bool Distortion = false;          // animated noise UV wobble (heat haze, licking flames)
    float DistortionStrength = 0.03f; // UV offset amplitude
    float DistortionScale = 1.5f;     // noise tiles per quad
    float DistortionScroll = 0.5f;    // noise scroll speed
};

// One emitter's worth of particles: what a single "piece" of an effect (flash, smoke, sparks) is.
struct ParticleLayer {
    std::string Name = "Layer";
    bool Enabled = true;
    ParticleEmission Emission;
    ParticleMotion Motion;
    ParticleAppearance Appearance;
    ParticleMaterial Material;

    // Clamps ranges/counts into valid values (min <= max, positive lifetimes, sane flipbook dims) - call
    // after loading or editing, so the simulation never has to defend against bad data per particle.
    void Validate();
};

// A complete effect: a composite of layers (e.g. Explosion = flash + fireball + smoke + sparks), loaded
// from a .particle file by ParticleLibrary or built in code. Shared via Ref - editing it updates every
// emitter using it and even particles already in flight.
struct ParticleEffect {
    std::string Name = "Effect";
    std::string SourcePath; // file it was loaded from (ParticleLibrary), empty for code-built effects
    std::vector<ParticleLayer> Layers;
    float Duration = 2.0f; // preview/loop length for the editor; gameplay ignores it

    void Validate() {
        for (ParticleLayer& layer : Layers)
            layer.Validate();
    }
};

} // namespace Wankel
