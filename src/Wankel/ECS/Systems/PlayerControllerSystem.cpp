
namespace Wankel {

class Scene;

	class PlayerControllerSystem::Update(Scene& scene, float dt) {
		auto& registry = scene.Registry();

        // Player Movement System
        auto view = m_Registry.view<TransformComponent, PlayerControllerComponent, RigidbodyComponent, MovementComponent>();

        for (auto entity : view) {

            auto& transform  = view.get<TransformComponent>(entity);
            auto& controller = view.get<PlayerControllerComponent>(entity);
            auto& rb         = view.get<RigidbodyComponent>(entity);
            auto& movement   = view.get<MovementComponent>(entity);

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
				
				// Movement TODO: is this needed here?:
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

			// TODO: this is a hack, this should be controlled from sandbox
            if (controller.Boost) {
				movement.MaxSpeed = movement.SavedMaxSpeed * controller.BoostMultiplier;
			}
			else { 
				movement.MaxSpeed = movement.SavedMaxSpeed;
			}
			movement.MoveIntent = moveDir;

            // APPLY TRANSFORM
            transform.LocalOrientation = controller.Orientation;
        }
	}

}
