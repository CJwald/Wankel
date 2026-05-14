#include "wkpch.h"
#include "Scene.h"
#include "Components.h"
#include "Wankel/Math/SecondOrderDynamics.h"
#include "Wankel/Renderer/Camera.h"

#include <glm/gtx/quaternion.hpp>

namespace Wankel {

    void Scene::OnUpdate(float dt, Camera& camera) {

        // Player Movement System
        auto view = m_Registry.view<
            TransformComponent,
            PlayerControllerComponent,
            RigidbodyComponent>();

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

        // PROCEDURAL MESH ANIMATION
        auto animView = m_Registry.view<TransformComponent, RigidbodyComponent, MeshAnimationComponent>();

        for (auto entity : animView) {

            auto& transform = animView.get<TransformComponent>(entity);
            auto& rb = animView.get<RigidbodyComponent>(entity);
            auto& anim = animView.get<MeshAnimationComponent>(entity);

            // VELOCITY IN LOCAL SPACE
            glm::vec3 localVelocity =
                glm::inverse(transform.LocalOrientation)
                * rb.Velocity;

            // TARGET POSITION OFFSET
            anim.TargetPosition =
                glm::vec3(
                    -localVelocity.x * anim.PositionAmplitude.x,
                    -localVelocity.y * anim.PositionAmplitude.y,
                    -localVelocity.z * anim.PositionAmplitude.z
                );

            // TARGET ROTATION OFFSET
            anim.TargetRotation =
                glm::vec3(
                    -localVelocity.y * anim.RotationAmplitude.x,   // pitch
                     localVelocity.x * anim.RotationAmplitude.y,   // yaw
                     localVelocity.x * anim.RotationAmplitude.z    // roll
                );

            // UPDATE SPRING SETTINGS
            anim.PositionSpring.SetDynamics(
                anim.PositionFrequency,
                anim.PositionDamping,
                anim.PositionResponse
            );

            anim.RotationSpring.SetDynamics(
                anim.RotationFrequency,
                anim.RotationDamping,
                anim.RotationResponse
            );

            // UPDATE SPRINGS
            anim.PositionOffset = anim.PositionSpring.Update(dt, anim.TargetPosition);
            anim.RotationOffset = anim.RotationSpring.Update(dt, anim.TargetRotation);
        }

        // FOLLOW CAMERA SYSTEM
        auto camView = m_Registry.view<TransformComponent, FollowCameraComponent>();

        for (auto entity : camView) {

            auto& transform = camView.get<TransformComponent>(entity);
            auto& follow = camView.get<FollowCameraComponent>(entity);

            if (!follow.Target)
                continue;

            auto& targetTransform = follow.Target.GetComponent<TransformComponent>();

            // BUILD TARGET TRANSFORM
            glm::mat4 targetMat =
                glm::translate(glm::mat4(1.0f), targetTransform.LocalPosition) *
                glm::toMat4(targetTransform.LocalOrientation);

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
}
