#include "wkpch.h"

#include "ReplicationSystem.h"

#include "ReplicationRegistry.h"
#include "Wankel/Networking/NetChannel.h"
#include "Wankel/Networking/NetSerialize.h"

namespace Wankel::Networking {

namespace {

// One row's worth of a single entity's replicated state - which row (index into
// GetReplicationTable()), which entity (by NetworkId), and that row's already-encoded bytes.
// Sent as its own EntitySnapshot message per row rather than batching every row for an entity into
// one message - simpler at this phase's entity/row counts, at the cost of a little extra envelope
// overhead per row.
struct EntitySnapshotBody {
    uint32_t NetworkIdValue = 0;
    uint8_t RowIndex = 0;
    NetBuffer RowPayload;
};

template <typename S>
void serialize(S& s, EntitySnapshotBody& b) {
    s.value4b(b.NetworkIdValue);
    s.value1b(b.RowIndex);
    s.container1b(b.RowPayload, 4096);
}

} // namespace

void ReplicationSystem::SnapshotAndSend(entt::registry& registry, NetHost& host, uint32_t tick) {
    const auto& table = GetReplicationTable();

    auto view = registry.view<NetworkId>();
    for (auto entity : view) {
        uint32_t id = view.get<NetworkId>(entity).Id;

        for (uint8_t rowIndex = 0; rowIndex < table.size(); rowIndex++) {
            NetBuffer rowBytes;
            if (!table[rowIndex].Snapshot(registry, entity, rowBytes))
                continue;

            EntitySnapshotBody body {id, rowIndex, std::move(rowBytes)};
            NetMessage msg = MakeMessage(NetMessageType::EntitySnapshot, tick, body);
            host.Broadcast(msg, NetChannel::UnreliableState, false);
        }
    }
}

entt::entity ReplicationSystem::FindEntityByNetworkId(entt::registry& registry, uint32_t id) {
    // Linear scan - fine at this phase's entity counts (single-digit replicated entities in the
    // demo). A real deployment would maintain an id->entity index alongside NetworkId instead.
    auto view = registry.view<NetworkId>();
    for (auto entity : view) {
        if (view.get<NetworkId>(entity).Id == id)
            return entity;
    }
    return entt::null;
}

void ReplicationSystem::RegisterHandlers(NetMessageBus& bus, entt::registry& registry) {
    bus.RegisterHandler(NetMessageType::EntitySnapshot, [this, &registry](NetPeer, const NetMessage& msg) {
        auto decoded = DecodeBody<EntitySnapshotBody>(msg.Payload);
        if (!decoded)
            return;

        entt::entity entity = FindEntityByNetworkId(registry, decoded->NetworkIdValue);

        // This connection owns this entity locally (its own player, say) - a broadcast snapshot for
        // it is stale by definition (enet_host_broadcast delivers to the sender too), so applying it
        // would fight whatever locally-authoritative state already updated it this frame.
        if (entity != entt::null && registry.get<NetworkId>(entity).IsLocallyOwned)
            return;

        if (entity == entt::null) {
            entity = registry.create();
            registry.emplace<NetworkId>(entity, NetworkId {decoded->NetworkIdValue, false});
        }

        const auto& table = GetReplicationTable();
        if (decoded->RowIndex >= table.size()) {
            WK_CORE_WARNING("ReplicationSystem: snapshot referenced unknown row {0}", decoded->RowIndex);
            return;
        }

        table[decoded->RowIndex].Apply(registry, entity, decoded->RowPayload);
    });
}

} // namespace Wankel::Networking
