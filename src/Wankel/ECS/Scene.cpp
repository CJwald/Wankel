#include "wkpch.h"
#include "Scene.h"
#include "Components.h"
#include "Wankel/Math/SecondOrderDynamics.h"
#include "Wankel/Renderer/Camera.h"

#include <glm/gtx/quaternion.hpp>

namespace Wankel {

	static void InitMeshAnimation(MeshAnimationComponent& anim) {
	    if (anim.Initialized)
	        return;

		for (int in = 0; in < MeshAnimationComponent::AxisCount; in++) {
    	    for (int out = 0; out < MeshAnimationComponent::AxisCount; out++) {
    	        auto& link = anim.Links[in][out];
    	        link.Spring = SecondOrderDynamics(link.Frequency, link.Damping, link.Response, 0.0f);
    	    }
    	}

    	anim.Initialized = true;
	}	


	static glm::mat4 ComposeTransform(const TransformComponent& tc) {
	    return glm::translate(glm::mat4(1.0f), tc.LocalPosition) *
	           glm::toMat4(tc.LocalOrientation) *
	           glm::scale(glm::mat4(1.0f), tc.LocalScale);
	}
	
	
	// RECURSIVE world transform resolver
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
			glm::vec3 newVel = (worldPos - tc.PreviousWorldPosition) / glm::max(dt, 1e-6f);
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


	void Scene::UpdateProceduralAnimation(float dt) {
	    auto view = m_Registry.view< TransformComponent, MeshAnimationComponent>();
	
	    for (auto entity : view) {
	        auto& tc = view.get<TransformComponent>(entity);
	        auto& anim = view.get<MeshAnimationComponent>(entity);
	
	        InitMeshAnimation(anim);

			glm::quat invRot = glm::inverse(tc.LocalOrientation);

        	glm::vec3 localVel = invRot * tc.WorldVelocity;
        	glm::vec3 localAngVel = invRot * tc.WorldAngularVelocity;

        	float input[(int)MotionAxis::Count] = {
        	    localVel.x, localVel.y, localVel.z,
        	    localAngVel.x, localAngVel.y, localAngVel.z
        	};
        	float output[(int)MotionAxis::Count] = {0};

        	for (int in = 0; in < (int)MotionAxis::Count; in++) {
        	    for (int out = 0; out < (int)MotionAxis::Count; out++) {
        	        auto& link = anim.Links[in][out];

        	        if (!link.Enabled)
        	            continue;

        	        float target = input[in] * link.Magnitude;
        	        target = glm::clamp(target, -link.Clamp, link.Clamp);

        	        link.Spring.SetDynamics(link.Frequency, link.Damping, link.Response);
        	        link.Output = link.Spring.Update(dt, target);
        	        output[out] += link.Output;
        	    }
        	}

        	anim.PositionOffset = glm::vec3(output[(int)MotionAxis::X], output[(int)MotionAxis::Y], output[(int)MotionAxis::Z]);
        	anim.RotationOffset = glm::vec3(output[(int)MotionAxis::Pitch], output[(int)MotionAxis::Yaw], output[(int)MotionAxis::Roll]);
        	tc.VisualPosition = anim.PositionOffset;

        	glm::vec3 rotRad = glm::radians(anim.RotationOffset);
        	tc.VisualRotation = glm::normalize(glm::quat(rotRad));	
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


    void Scene::OnUpdate(float dt, Camera& camera) {
		

        // Player Movement System
        auto view = m_Registry.view< TransformComponent, PlayerControllerComponent, RigidbodyComponent>();

        for (auto entity : view) {

            auto& transform  = view.get<TransformComponent>(entity);
            auto& controller = view.get<PlayerControllerComponent>(entity);
            auto& rb         = view.get<RigidbodyComponent>(entity);

            // INPUT
            float dx = controller.LookDeltaX * controller.WindowSensitivity;
            float dy = controller.LookDeltaY * controller.WindowSensitivity;

            float rollMag = controller.RollInput;

            rollMag = glm::clamp(rollMag, -1.0f, 1.0f);

            glm::vec3 forward = controller.Orientation * glm::vec3(0,0,-1);
            glm::vec3 right   = controller.Orientation * glm::vec3(1,0,0);
            glm::vec3 up      = controller.Orientation * glm::vec3(0,1,0);

            // FPS LOOK MODE
            if (controller.Mode == PlayerControllerComponent::LookMode::FPS) {
				
            	glm::vec3 bodyForward = controller.BodyOrientation * glm::vec3(0,0,-1);
            	glm::vec3 bodyRight   = controller.BodyOrientation * glm::vec3(1,0,0);
            	glm::vec3 bodyUp      = controller.BodyOrientation * glm::vec3(0,1,0);

				float yawDelta   = -dx * controller.MouseSensitivity;
				float pitchDelta = -dy * controller.MouseSensitivity;
				float rollDelta  = rollMag * controller.RollSpeed * dt;
                controller.Pitch += pitchDelta;

                // CLAMP PITCH
                controller.Pitch = glm::clamp( controller.Pitch, controller.MaxPitchDown, controller.MaxPitchUp );

                glm::quat yawQuat = glm::angleAxis( yawDelta, bodyUp );
                glm::quat rollQuat = glm::angleAxis( rollDelta, bodyForward );
				controller.BodyOrientation = glm::normalize( rollQuat * yawQuat * controller.BodyOrientation ); 
                bodyRight = controller.BodyOrientation * glm::vec3(1,0,0);

                glm::quat pitchQuat = glm::angleAxis( controller.Pitch, bodyRight );
				controller.Orientation = glm::normalize( pitchQuat * controller.BodyOrientation );
				
				// Movement TODO: is this needed here:
                forward = controller.BodyOrientation * glm::vec3(0,0,-1);
                right =   controller.BodyOrientation * glm::vec3(1,0,0);
                up =      controller.BodyOrientation * glm::vec3(0,1,0);
            }

            // FLIGHT MODE
            else if (controller.Mode == PlayerControllerComponent::LookMode::Flight) {

				// MOVEMENT TODO: is this needed here?
                forward = controller.Orientation * glm::vec3(0,0,-1);
                right =   controller.Orientation * glm::vec3(1,0,0);
                up =      controller.Orientation * glm::vec3(0,1,0);

				// LOOK
                glm::quat yaw = glm::angleAxis( -dx * controller.MouseSensitivity, up );
                glm::quat pitch = glm::angleAxis( -dy * controller.MouseSensitivity, right );
                glm::quat roll = glm::angleAxis( rollMag * controller.RollSpeed * dt, forward );

                controller.Orientation = glm::normalize( roll * pitch * yaw * controller.Orientation );
            }

            glm::vec3 moveDir = controller.MoveInput;
            moveDir = moveDir.z * forward + moveDir.x * right + moveDir.y * up;

            if (glm::length(moveDir) > 0.0f) {
                moveDir = glm::normalize(moveDir);
			}

            float speed = controller.MoveSpeed;

            if (controller.Boost) {
                speed *= controller.BoostMultiplier;
			}

            rb.ForcedVelocity = moveDir * speed;

            // APPLY TRANSFORM
            transform.LocalOrientation = controller.Orientation;
        }

        // PHYSICS
        m_PhysicsSystem.Update(*this, dt);

        // UPDATES
		UpdateTransforms(dt);
		UpdateProceduralAnimation(dt);
		UpdateFinalTransforms();

        // FOLLOW CAMERA SYSTEM
        auto camView = m_Registry.view<TransformComponent, FollowCameraComponent>();

        for (auto entity : camView) {
            //auto& transform = camView.get<TransformComponent>(entity);
            auto& follow = camView.get<FollowCameraComponent>(entity);

            if (!follow.Target)
                continue;

            auto& targetTransform = follow.Target.GetComponent<TransformComponent>();

            // BUILD TARGET TRANSFORM
			glm::mat4 targetMat = targetTransform.WorldTransform;

            // BUILD CAMERA OFFSET
            glm::mat4 offsetMat =
                glm::translate(glm::mat4(1.0f), follow.Offset) *
                glm::mat4_cast(follow.RotationOffset);

            // FINAL CAMERA TRANSFORM
            glm::mat4 cameraMat = targetMat * offsetMat;

            // POSITION
            glm::vec3 cameraPos = glm::vec3(cameraMat[3]);
            camera.SetPosition(cameraPos);

            // ROTATION
            glm::mat4 rotMat = cameraMat;
            rotMat[3] = glm::vec4(0,0,0,1);
            glm::quat cameraRot = glm::quat_cast(rotMat);
            camera.SetOrientation(cameraRot);
        }
    }


	static float GetAxisValue(MotionAxis axis, const glm::vec3& pos, const glm::vec3& rot) {
	    switch (axis) {
	        case MotionAxis::X: return pos.x;
	        case MotionAxis::Y: return pos.y;
	        case MotionAxis::Z: return pos.z;
	
	        case MotionAxis::Pitch: return rot.x;
	        case MotionAxis::Yaw: return rot.y;
	        case MotionAxis::Roll: return rot.z;
	    }
	
	    return 0.0f;
	}


}
