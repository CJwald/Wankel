#pragma once

#include "Wankel/Core/Base.h"
#include "Wankel/Particles/ParticleEffect.h"

namespace Wankel::ParticleEffects {

// Built-in single-layer presets on the default soft sprite - quick defaults and code examples. Authored
// effects normally live in .particle files (ParticleLibrary::Load). Each call returns a fresh effect.
Ref<ParticleEffect> Smoke();
Ref<ParticleEffect> Sparks();
Ref<ParticleEffect> Blood();
Ref<ParticleEffect> MuzzleFlash();

} // namespace Wankel::ParticleEffects
