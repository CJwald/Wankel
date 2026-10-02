#pragma once

#include "Wankel/Core/Base.h"
#include "Wankel/Particles/ParticleEffect.h"

#include <nlohmann/json.hpp>

#include <string>

namespace Wankel {

// Loads/saves ParticleEffects as human-readable JSON (.particle) and caches them by path, so every caller
// asking for the same file shares one Ref - an edit (or Reload) updates all of them at once, including
// particles already in flight.
namespace ParticleLibrary {

// Cached; nullptr (with an error logged) if the file is missing or malformed. The result is Validate()d.
Ref<ParticleEffect> Load(const std::string& path);

// Writes `effect` to `path` (creating directories as needed). False on I/O failure.
bool Save(const ParticleEffect& effect, const std::string& path);

// Re-reads `path` into its already-cached effect in place (holders keep their Ref). Loads it if uncached.
bool Reload(const std::string& path);

nlohmann::json ToJson(const ParticleEffect& effect);
// Missing fields keep their defaults, so older/hand-written files stay loadable.
ParticleEffect FromJson(const nlohmann::json& json);

// Drops one path from the cache (e.g. after deleting its file), so a later Load doesn't return the stale effect.
void Forget(const std::string& path);

// Drops the cache (holders keep their Refs).
void Clear();

} // namespace ParticleLibrary

} // namespace Wankel
