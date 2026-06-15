#pragma once
#include "Entity.h"
#include "Wankel/Renderer/Camera.h"
#include "Wankel/Physics/Systems/PhysicsSystem.h"

#include "Systems/PlayerControllerSystem.h"
#include "Systems/TransformSystem.h"
#include "Systems/ProceduralAnimationSystem.h"
#include "Systems/CameraSystem.h"

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
    	void UpdateKinematics(float dt);

    private:
		PlayerControllerSystem m_PlayerControllerSystem;
		TransformSystem m_TransformSystem;
		ProceduralAnimationSystem m_ProceduralAnimationSystem;
		CameraSystem m_CameraSystem;

        entt::registry m_Registry;
		PhysicsSystem m_PhysicsSystem;
    };

}
