#include "wkpch.h"
#include "Scene.h"
#include "Components.h"

#include "Wankel/Core/Input.h"
#include "Wankel/Core/KeyCodes.h"
#include "Wankel/Renderer/Camera.h"

#include <glm/gtx/quaternion.hpp>

namespace Wankel {

    void Scene::OnUpdate(float dt, Camera& camera) {

        // =========================
        // Player Movement System
        // =========================
		auto view = m_Registry.view<TransformComponent, PlayerControllerComponent, RigidbodyComponent>();

        for (auto entity : view) {
            auto& transform = view.get<TransformComponent>(entity);
            auto& controller = view.get<PlayerControllerComponent>(entity);
			auto& rb = view.get<RigidbodyComponent>(entity);

			// these may need testing to fix
            glm::vec3 forward = controller.Orientation * glm::vec3(0,0,-1);
            glm::vec3 right   = controller.Orientation * glm::vec3(1,0,0);
            glm::vec3 up      = controller.Orientation * glm::vec3(0,1,0);
			
			glm::vec3 moveDir(0.0f);
			
        	if (Input::IsKeyPressed(Key::W)) moveDir += forward;
        	if (Input::IsKeyPressed(Key::S)) moveDir -= forward;
        	if (Input::IsKeyPressed(Key::D)) moveDir += right;
        	if (Input::IsKeyPressed(Key::A)) moveDir -= right;
        	if (Input::IsKeyPressed(Key::Space)) moveDir += up;
        	if (Input::IsKeyPressed(Key::LeftControl)) moveDir -= up;
		
			if (glm::length(moveDir) > 0.0f)
        	    moveDir = glm::normalize(moveDir);

			rb.ForcedVelocity = moveDir * controller.MoveSpeed;

        	// =========================
        	// MOUSE LOOK
        	// =========================
            float dx = Input::GetMouseDeltaX() * controller.WindowSensitivity;
            float dy = Input::GetMouseDeltaY() * controller.WindowSensitivity;

	    	if (dx != 0.0f || dy != 0.0f) {
	    	    // --- YAW (world up, always stable) ---
	    	    glm::quat yaw = glm::angleAxis(
	    	        -dx * controller.MouseSensitivity,
	    	        up
	    	    );
	    	    controller.Orientation = glm::normalize(yaw * controller.Orientation);

	    	    // --- recompute right AFTER yaw ---
	    	    glm::vec3 newRight = controller.Orientation * glm::vec3(1, 0, 0);

	    	    // --- PITCH (local right, stable now) ---
	    	    glm::quat pitch = glm::angleAxis(
	    	        -dy * controller.MouseSensitivity,
	    	        newRight
	    	    );
	    	    controller.Orientation = glm::normalize(pitch * controller.Orientation);
	    	}

	    	// ========================
	    	// Roll (local forward)
	    	// ========================
	    	glm::vec3 currentForward = controller.Orientation * glm::vec3(0, 0, -1);

	    	if (Input::IsKeyPressed(Key::E))
	    	    controller.Orientation = glm::normalize(
	    	        glm::angleAxis(controller.RollSpeed * dt, currentForward) * controller.Orientation 
	    	    );

	    	if (Input::IsKeyPressed(Key::Q))
	    	    controller.Orientation = glm::normalize(
	    	        glm::angleAxis(-controller.RollSpeed * dt, currentForward) * controller.Orientation 
	    	    );

			transform.Orientation = controller.Orientation;
		}

		m_PhysicsSystem.Update(*this, dt);

        // =========================
        // Follow Camera System
        // =========================
        auto camView = m_Registry.view<TransformComponent, FollowCameraComponent>();

        for (auto entity : camView) {
            auto& transform = camView.get<TransformComponent>(entity);
            auto& follow = camView.get<FollowCameraComponent>(entity);

            if (!follow.Target)
                continue;

            auto& targetTransform = follow.Target.GetComponent<TransformComponent>();

			// =========================
    		// BUILD TARGET TRANSFORM
    		// =========================
    		glm::mat4 targetMat =
    		    glm::translate(glm::mat4(1.0f), targetTransform.Position) *
    		    glm::toMat4(targetTransform.Orientation);

    		// =========================
    		// BUILD CAMERA OFFSET
    		// =========================
    		glm::mat4 offsetMat =
    		    glm::translate(glm::mat4(1.0f), follow.Offset) *
    		    glm::mat4_cast(follow.RotationOffset);

    		// =========================
    		// FINAL CAMERA TRANSFORM
    		// =========================
    		glm::mat4 cameraMat = targetMat * offsetMat;

    		// =========================
    		// EXTRACT POSITION
    		// =========================
    		glm::vec3 cameraPos = glm::vec3(cameraMat[3]);
    		camera.SetPosition(cameraPos);

    		// =========================
    		// EXTRACT ROTATION
    		// =========================
    		glm::mat4 rotMat = cameraMat;
    		rotMat[3] = glm::vec4(0, 0, 0, 1);

    		glm::quat cameraRot = glm::quat_cast(rotMat);
    		camera.SetOrientation(cameraRot);
        }

    }

}
