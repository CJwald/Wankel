#include "wkpch.h"
#include "Scene.h"
#include "Components.h"


namespace Wankel {

    void Scene::OnUpdate(float dt, Camera& camera) {

		m_PlayerControllerSystem.Update(*this, dt);
    	m_PhysicsSystem.Update(*this, dt);
    	m_KinematicsSystem.Update(*this, dt);
    	m_ProceduralAnimationSystem.Update(*this, dt);
    	m_CameraSystem.Update(*this, camera);

	}
}
