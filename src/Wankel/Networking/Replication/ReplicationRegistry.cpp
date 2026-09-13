#include "wkpch.h"

#include "ReplicationRegistry.h"

#include "Wankel/ECS/Components/PhysicsComponents.h"
#include "Wankel/ECS/Components/TransformComponents.h"
#include "Wankel/Networking/NetSerialize.h"

namespace Wankel::Networking {

namespace {

// Minimal wire shape for Transform - position + orientation only. LocalScale, the visual
// offset fields, and the cached matrices either don't change per-tick or are purely local render
// state, so they never need to cross the wire.
struct NetTransformSnapshot {
    glm::vec3 Position {0.0f};
    glm::quat Orientation {1, 0, 0, 0};
};

template <typename S>
void serialize(S& s, NetTransformSnapshot& t) {
    s.value4b(t.Position.x);
    s.value4b(t.Position.y);
    s.value4b(t.Position.z);
    s.value4b(t.Orientation.w);
    s.value4b(t.Orientation.x);
    s.value4b(t.Orientation.y);
    s.value4b(t.Orientation.z);
}

struct NetVelocitySnapshot {
    glm::vec3 Velocity {0.0f};
};

template <typename S>
void serialize(S& s, NetVelocitySnapshot& v) {
    s.value4b(v.Velocity.x);
    s.value4b(v.Velocity.y);
    s.value4b(v.Velocity.z);
}

} // namespace

const std::vector<ReplicatedComponentEntry>& GetReplicationTable() {
    static const std::vector<ReplicatedComponentEntry> table = {
        ReplicatedComponentEntry {
            "Transform",
            [](entt::registry& reg, entt::entity e, NetBuffer& out) {
                auto* transform = reg.try_get<Transform>(e);
                if (!transform)
                    return false;
                NetTransformSnapshot snapshot {transform->LocalPosition, transform->LocalOrientation};
                out = EncodeBytes(snapshot);
                return true;
            },
            [](entt::registry& reg, entt::entity e, const NetBuffer& in) {
                auto decoded = DecodeBody<NetTransformSnapshot>(in);
                if (!decoded)
                    return;
                Transform& transform = reg.get_or_emplace<Transform>(e);
                transform.LocalPosition = decoded->Position;
                transform.LocalOrientation = decoded->Orientation;
            },
        },
        ReplicatedComponentEntry {
            "Rigidbody.Velocity",
            [](entt::registry& reg, entt::entity e, NetBuffer& out) {
                auto* rigidbody = reg.try_get<Rigidbody>(e);
                if (!rigidbody)
                    return false;
                NetVelocitySnapshot snapshot {rigidbody->Velocity};
                out = EncodeBytes(snapshot);
                return true;
            },
            [](entt::registry& reg, entt::entity e, const NetBuffer& in) {
                auto decoded = DecodeBody<NetVelocitySnapshot>(in);
                if (!decoded)
                    return;
                reg.get_or_emplace<Rigidbody>(e).Velocity = decoded->Velocity;
            },
        },
    };
    return table;
}

} // namespace Wankel::Networking
