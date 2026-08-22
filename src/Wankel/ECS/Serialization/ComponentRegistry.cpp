#include "wkpch.h"

#include "ComponentRegistry.h"

#include "ComponentSerialization.h"
#include "Wankel/ECS/Components.h"

namespace Wankel::Serialization {

namespace {

template <typename T>
ComponentEntry MakeEntry(const char* key) {
    return ComponentEntry {
        key,
        [](entt::registry& reg, entt::entity e, nlohmann::json& out) {
            auto* component = reg.try_get<T>(e);
            if (!component)
                return false;
            out = Serialize(*component);
            return true;
        },
        [](entt::registry& reg, entt::entity e, const nlohmann::json& in) {
            Deserialize(in, reg.get_or_emplace<T>(e));
        },
    };
}

} // namespace

const std::vector<ComponentEntry>& GetComponentTable() {
    static const std::vector<ComponentEntry> table = {
        MakeEntry<Transform>("Transform"),         MakeEntry<SphereCollider>("SphereCollider"),
        MakeEntry<AABBCollider>("AABBCollider"),   MakeEntry<CapsuleCollider>("CapsuleCollider"),
        MakeEntry<MeshAnimation>("MeshAnimation"), MakeEntry<PlayerController>("PlayerController"),
        MakeEntry<Rigidbody>("Rigidbody"),         MakeEntry<Movement>("Movement"),
        MakeEntry<MeshRenderer>("MeshRenderer"),
    };
    return table;
}

} // namespace Wankel::Serialization
