#pragma once

#include "ReplicationComponents.h"
#include "Wankel/Networking/NetDispatch.h"
#include "Wankel/Networking/NetHost.h"

#include <entt/entt.hpp>

#include <cstdint>

namespace Wankel::Networking {

// Sits at the Layer level (like MechtrixLayer already drives PlayerInputSystem around
// Scene::OnUpdate), not inside Scene's fixed system pipeline - keeps core ECS/Scene
// networking-agnostic and usable in a fully offline build with zero Networking/ code linked.
class ReplicationSystem {
public:
    // Call once per tick, immediately after Scene::OnUpdate, on the authoritative side (a server -
    // broadcast-to-everyone is the whole story at this phase's dozens-~100 player target, no
    // per-peer relevancy filtering yet). Walks every NetworkId entity in `registry`, snapshots each
    // replicated component via GetReplicationTable(), and broadcasts the result over `host` on the
    // unreliable-sequenced channel.
    void SnapshotAndSend(entt::registry& registry, NetHost& host, uint32_t tick);

    // Wires the EntitySnapshot handler into `bus` so incoming snapshots get applied to the matching
    // NetworkId entity in `registry` (creating it on first sight). NetMessageBus already guarantees
    // this runs on the main thread, so it's safe to touch `registry` here - applied state is
    // visible to the *next* frame's Scene::OnUpdate.
    void RegisterHandlers(NetMessageBus& bus, entt::registry& registry);

private:
    entt::entity FindEntityByNetworkId(entt::registry& registry, uint32_t id);
};

} // namespace Wankel::Networking
