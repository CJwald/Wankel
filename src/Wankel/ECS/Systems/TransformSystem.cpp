

namespace Wankel {

	void TransformSystem::Update(Scene& scene, float dt) {
		UpdateTransforms(scene, dt);
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


	void Scene::UpdateTransforms(float dt) {

	    auto view = m_Registry.view<TransformComponent>();
	
	    for (auto entity : view) {
	        auto& tc = view.get<TransformComponent>(entity);
			tc.LocalTransform = ComposeTransform(tc);
        	tc.WorldTransform = ComputeWorldTransform(m_Registry, entity);

        	glm::vec3 worldPos = glm::vec3(tc.WorldTransform[3]);
			glm::vec3 newVel = {0.f, 0.f,0.f};
			if (glm::length(worldPos - tc.PreviousWorldPosition) < m_dPosThreshold) {
				newVel = (worldPos - tc.PreviousWorldPosition) / glm::max(dt, 1e-6f);
			} 
			else {
        		newVel = tc.WorldVelocity;
			}
        	tc.WorldAcceleration = (newVel - tc.WorldVelocity) / glm::max(dt, 1e-6f);
        	tc.WorldVelocity = newVel;

        	glm::quat currentRot = glm::quat_cast(tc.WorldTransform);
        	glm::quat delta = currentRot * glm::inverse(tc.PreviousWorldRotation);

        	float angle = glm::angle(delta);
        	if (angle > glm::pi<float>())
        	    angle -= glm::two_pi<float>();

        	glm::vec3 axis(0.0f);
        	if (glm::abs(angle) > 0.00001f)
        	    axis = glm::axis(delta);

			glm::vec3 newAngVel = axis * (angle / glm::max(dt, 1e-6f));
        	tc.WorldAngularAcceleration = (newAngVel - tc.WorldAngularVelocity) / glm::max(dt, 1e-6f);
        	tc.WorldAngularVelocity = newAngVel;
        	tc.PreviousWorldPosition = worldPos;
        	tc.PreviousWorldRotation = currentRot;
	    }
	}


	void Scene::UpdateFinalTransforms() {
	    auto view = m_Registry.view<TransformComponent>();
	    for (auto entity : view) {
	        auto& tc = view.get<TransformComponent>(entity);

			glm::mat4 visual = glm::translate(glm::mat4(1.0f), tc.VisualPosition) * glm::toMat4(tc.VisualRotation);

        	// APPLY VISUAL IN LOCAL SPACE
        	tc.FinalTransform = tc.WorldTransform * visual;
	    }
	}






}
