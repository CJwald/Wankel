#pragma once

#include "Wankel/Particles/ParticleEffect.h"

namespace Wankel::ParticleEffects {

// Ready-to-use presets built on the shared particle implementation - copy one into a
// ParticleEmitter::Effect and tweak from there. Values are deliberately conservative; tune per game.
ParticleEffect Smoke();
ParticleEffect Sparks();
ParticleEffect Blood();
ParticleEffect MuzzleFlash();

} // namespace Wankel::ParticleEffects
