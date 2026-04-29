#pragma once
#include "Entity.h"
//#include <entt/entt.hpp>

namespace Wankel {

    class Scene {
    public:
        Entity CreateEntity() {
            return Entity(m_Registry.create(), &m_Registry);
        }

        void DestroyEntity(Entity entity) {
            m_Registry.destroy(entity.GetHandle());
        }

        entt::registry& Registry() { return m_Registry; }

    private:
        entt::registry m_Registry;
    };

}
