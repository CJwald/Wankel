#pragma once
#include <entt/entt.hpp>

namespace Wankel {

    class Entity {
    public:
        Entity() = default;
        Entity(entt::entity handle, entt::registry* registry)
            : m_EntityHandle(handle), m_Registry(registry) {}

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args) {
            return m_Registry->emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
        }

        template<typename T>
        T& GetComponent() {
            return m_Registry->get<T>(m_EntityHandle);
        }

        template<typename T>
        bool HasComponent() {
            return m_Registry->all_of<T>(m_EntityHandle);
        }

        operator bool() const { return m_EntityHandle != entt::null; }

        entt::entity GetHandle() const { return m_EntityHandle; }

    private:
        entt::entity m_EntityHandle{ entt::null };
        entt::registry* m_Registry = nullptr;
    };

}
