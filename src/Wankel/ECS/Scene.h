#pragma once
#include "Entity.h"
#include "Wankel/Physics/Systems/PhysicsSystem.h"
#include "Wankel/ECS/Systems/PlayerControllerSystem.h"
#include "Wankel/ECS/Systems/TransformSystem.h"
#include "Wankel/ECS/Systems/KinematicsSystem.h"
#include "Wankel/ECS/Systems/ProceduralAnimationSystem.h"
#include "Wankel/ECS/Systems/CameraSystem.h"
#include "Wankel/Renderer/Camera.h"

namespace Wankel {

    class Scene {
    public:
        Entity CreateEntity() {
            return Entity(m_Registry.create(), &m_Registry);
        }

        void DestroyEntity(Entity entity) {
            m_Registry.destroy(entity.GetHandle());
        }

		void OnUpdate(float dt, Camera& camera);

        entt::registry& Registry() { return m_Registry; }

    private:
		PlayerControllerSystem m_PlayerControllerSystem;
		TransformSystem m_TransformSystem;
		KinematicsSystem m_KinematicsSystem;
		ProceduralAnimationSystem m_ProceduralAnimationSystem;
		CameraSystem m_CameraSystem;

        entt::registry m_Registry;
		PhysicsSystem m_PhysicsSystem;
    };

}
