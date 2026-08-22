#pragma once

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

#include <functional>
#include <vector>

namespace Wankel::Serialization {

// Serialize returns false (and leaves the json untouched) if the entity doesn't have the component.
// Deserialize adds the component (get_or_emplace) if missing, then applies the saved fields to it.
struct ComponentEntry {
    const char* Key;
    std::function<bool(entt::registry&, entt::entity, nlohmann::json&)> Serialize;
    std::function<void(entt::registry&, entt::entity, const nlohmann::json&)> Deserialize;
};

// One row per Wankel-owned component this feature can save/load - see ComponentSerialization.h for
// which components are covered and why (tuning-only: no live velocities/cooldowns/spring state/...).
// Mirrors CollisionDispatcher's table-dispatch style (Physics/Collision/CollisionDispatcher.cpp)
// rather than a macro/reflection system - every row is a plain, greppable line naming its type.
// Mechtrix-owned gameplay components (Health/Faction/MobController/Weapon) deliberately have no entry
// here - they get their own parallel table on the game side, composed alongside this one rather than
// mixed into it.
const std::vector<ComponentEntry>& GetComponentTable();

} // namespace Wankel::Serialization
