#include "wkpch.h"
#include "TransformSystem.h"

#include "Wankel/ECS/Scene.h"
#include "Wankel/ECS/Components.h"

#include <glm/gtx/quaternion.hpp>



namespace Wankel {

	static glm::mat4 ComposeTransform(const Transform& tc) {
	    return glm::translate(glm::mat4(1.0f), tc.LocalPosition) *
	           glm::toMat4(tc.LocalOrientation) *
	           glm::scale(glm::mat4(1.0f), tc.LocalScale);
	}


	static glm::mat4 ComputeWorldTransform(entt::registry& registry, entt::entity e) {

	    auto& tc = registry.get<Transform>(e);
	    glm::mat4 local = ComposeTransform(tc);
	
	    if (registry.all_of<Parent>(e)) {
	        auto parent = registry.get<Parent>(e).Parent.GetHandle();
	        if (parent != entt::null) {
	            glm::mat4 parentWorld = ComputeWorldTransform(registry, parent);
	            return parentWorld * local;
	        }
	    }
	
	    return local;
	}


	void TransformSystem::Update(Scene& scene) {
	    auto& registry = scene.Registry();
	    auto view = registry.view<Transform>();
	
	    for (auto entity : view) {
	        auto& tc = view.get<Transform>(entity);
	
	        tc.LocalTransform = ComposeTransform(tc);
	        tc.WorldTransform = ComputeWorldTransform(registry, entity);
	
	        glm::mat4 visual = glm::translate(glm::mat4(1.0f), tc.VisualPosition) * glm::toMat4(tc.VisualRotation);
	
	        tc.FinalTransform = tc.WorldTransform * visual;
	    }
	}
	

	void TransformSystem::UpdateFinalTransforms(Scene& scene) {
		auto& registry = scene.Registry();
		auto view = registry.view<Transform>();

	    for (auto entity : view) {
	        auto& tc = view.get<Transform>(entity);
			glm::mat4 visual = glm::translate(glm::mat4(1.0f), tc.VisualPosition) * glm::toMat4(tc.VisualRotation);

        	// APPLY VISUAL IN LOCAL SPACE
        	tc.FinalTransform = tc.WorldTransform * visual;
	    }
	}

}
