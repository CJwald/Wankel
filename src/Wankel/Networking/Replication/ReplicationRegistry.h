#pragma once

#include "Wankel/Networking/NetMessage.h"

#include <entt/entt.hpp>

#include <functional>
#include <vector>

namespace Wankel::Networking {

// Bitsery analog of Wankel::Serialization::ComponentEntry
// (ECS/Serialization/ComponentRegistry.h), but for LIVE runtime state carried over the wire every
// tick (position, velocity, aim) rather than tuning-only JSON snapshots.
//
// Engine-owned rows only. Mechtrix-owned gameplay components (Health, Weapon::RecoilSeed/
// AmmoRemaining, Faction) get their own parallel table on the game side, composed alongside this
// one rather than merged into it - exactly like ComponentRegistry's own documented pattern, so
// Wankel never needs to know Mechtrix's component types.
//
// Snapshot returns false (and leaves the buffer untouched) if the entity doesn't have the
// component. Apply adds the component (get_or_emplace) if missing, then writes the decoded fields
// onto it - mirrors ComponentEntry::Serialize/Deserialize exactly.
struct ReplicatedComponentEntry {
    const char* Key;
    std::function<bool(entt::registry&, entt::entity, NetBuffer&)> Snapshot;
    std::function<void(entt::registry&, entt::entity, const NetBuffer&)> Apply;
};

const std::vector<ReplicatedComponentEntry>& GetReplicationTable();

} // namespace Wankel::Networking
