#include "wkpch.h"
#include "KinematicsSystem.h"

#include "Wankel/ECS/Scene.h"
#include "Wankel/ECS/Components.h"

#include <glm/gtx/quaternion.hpp>


namespace Wankel {

	void KinematicsSystem::Update(Scene& scene, float dt) {
		UpdateKinematics(scene, dt);
		UpdateFinalTransforms(scene);
	}


	static glm::mat4 ComposeTransform(const TransformComponent& tc) {
	    return glm::translate(glm::mat4(1.0f), tc.LocalPosition) *
	           glm::toMat4(tc.LocalOrientation) *
	           glm::scale(glm::mat4(1.0f), tc.LocalScale);
	}


	static glm::mat4 ComputeWorldTransform(entt::registry& registry, entt::entity e) {

	    auto& tc = registry.get<TransformComponent>(e);
	    glm::mat4 local = ComposeTransform(tc);
	
	    if (registry.all_of<ParentComponent>(e)) {
	        auto parent = registry.get<ParentComponent>(e).Parent.GetHandle();
	        if (parent != entt::null) {
	            glm::mat4 parentWorld = ComputeWorldTransform(registry, parent);
	            return parentWorld * local;
	        }
	    }
	
	    return local;
	}


	void KinematicsSystem::UpdateKinematics(Scene& scene, float dt) {

		auto& registry = scene.Registry();
		auto view = registry.view<TransformComponent, KinematicsComponent>();
	
	    for (auto entity : view) {
	        auto& tc = view.get<TransformComponent>(entity);
	        auto& kc = view.get<KinematicsComponent>(entity);
			tc.LocalTransform = ComposeTransform(tc);
        	tc.WorldTransform = ComputeWorldTransform(registry, entity);

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


	void KinematicsSystem::UpdateFinalTransforms(Scene& scene) {
		auto& registry = scene.Registry();
		auto view = registry.view<TransformComponent>();

	    for (auto entity : view) {
	        auto& tc = view.get<TransformComponent>(entity);

			glm::mat4 visual = glm::translate(glm::mat4(1.0f), tc.VisualPosition) * glm::toMat4(tc.VisualRotation);

        	// APPLY VISUAL IN LOCAL SPACE
        	tc.FinalTransform = tc.WorldTransform * visual;
	    }
	}
}
