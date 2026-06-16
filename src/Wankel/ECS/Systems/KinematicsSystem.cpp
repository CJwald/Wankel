#include "wkpch.h"
#include "KinematicsSystem.h"

#include "Wankel/ECS/Scene.h"
#include "Wankel/ECS/Components.h"

#include <glm/gtx/quaternion.hpp>


namespace Wankel {

	void KinematicsSystem::Update(Scene& scene, float dt) {

		auto& registry = scene.Registry();
		auto view = registry.view<TransformComponent, KinematicsComponent>();
	
	    for (auto entity : view) {
	        auto& tc = view.get<TransformComponent>(entity);
	        auto& kc = view.get<KinematicsComponent>(entity);
			//tc.LocalTransform = ComposeTransform(tc);
        	//tc.WorldTransform = ComputeWorldTransform(registry, entity);

        	glm::vec3 worldPos = glm::vec3(tc.WorldTransform[3]);
			glm::vec3 newVel = {0.f, 0.f,0.f};
			if (glm::length(worldPos - kc.PreviousWorldPosition) < m_dPosThreshold) {
				newVel = (worldPos - kc.PreviousWorldPosition) / glm::max(dt, 1e-6f);
			} 
			else {
        		newVel = kc.WorldVelocity;
			}
        	kc.WorldAcceleration = (newVel - kc.WorldVelocity) / glm::max(dt, 1e-6f);
        	kc.WorldVelocity = newVel;

        	glm::quat currentRot = glm::quat_cast(tc.WorldTransform);
        	glm::quat delta = currentRot * glm::inverse(kc.PreviousWorldRotation);

        	float angle = glm::angle(delta);
        	if (angle > glm::pi<float>())
        	    angle -= glm::two_pi<float>();

        	glm::vec3 axis(0.0f);
        	if (glm::abs(angle) > 0.00001f)
        	    axis = glm::axis(delta);

			glm::vec3 newAngVel = axis * (angle / glm::max(dt, 1e-6f));
        	kc.WorldAngularAcceleration = (newAngVel - kc.WorldAngularVelocity) / glm::max(dt, 1e-6f);
        	kc.WorldAngularVelocity = newAngVel;
        	kc.PreviousWorldPosition = worldPos;
        	kc.PreviousWorldRotation = currentRot;
	    }
	}
}
