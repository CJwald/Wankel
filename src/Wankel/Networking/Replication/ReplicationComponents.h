#pragma once

#include <cstdint>

namespace Wankel::Networking {

// Stable cross-peer entity identifier - entt::entity handles are process-local, so a replicated
// entity needs an id both sides agree on. The authoritative side (server, or a client for its own
// locally-owned entities) assigns Id when the entity is created; the other side learns it via an
// EntitySpawn message and uses it to find/create the matching local entity.
struct NetworkId {
    uint32_t Id = 0;
    bool IsLocallyOwned = false; // client-side only: true for the local player's own entity
};

} // namespace Wankel::Networking
