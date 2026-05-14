#include "PlayerInputSystem.h"

#include <Wankel/Core/Input.h>
#include <Wankel/Core/ControllerInput.h>
#include <Wankel/Core/KeyCodes.h>
#include <Wankel/ECS/Components.h>
#include <Wankel/Core/Log.h>

namespace Wankel {

void PlayerInputSystem::Update(Scene& scene, float dt, bool gameFocused)
{
    auto controllerView =
        scene.Registry().view<PlayerControllerComponent>();

    for (auto entity : controllerView)
    {
        // Skip if game not focused
        if (!gameFocused)
            continue;

        auto& controller =
            controllerView.get<PlayerControllerComponent>(entity);

        int pad = 0;
        
        // =========================
		// LOOK MODE TOGGLE (R3)
		// =========================

		static bool r3PressedLastFrame = false;
		bool r3Pressed = false;

        if (Input::IsKeyPressed(Key::X) || ControllerInput::IsButtonPressed(pad, GamepadButton::R3)) {
			r3Pressed = true;
		}
		//bool r3Pressed = ControllerInput::IsButtonPressed(pad, GamepadButton::R3);

		if (r3Pressed && !r3PressedLastFrame)
		{
			if (controller.Mode ==
				PlayerControllerComponent::LookMode::FPS)
			{
				controller.Mode =
				    PlayerControllerComponent::LookMode::Flight;
			}
			else
			{
				controller.Mode =
				    PlayerControllerComponent::LookMode::FPS;

				// =====================================
				// Rebuild FPS yaw/pitch from orientation
				// so transitions feel smooth
				// =====================================

				//glm::vec3 forward =
				//    controller.Orientation *
				//    glm::vec3(0,0,-1);

				//controller.Yaw =
				//    atan2(forward.x, -forward.z);

				//controller.Pitch =
				//    asin(glm::clamp(
				//        forward.y,
				//        -1.0f,
				//        1.0f
				//    ));
			}
		}

		r3PressedLastFrame = r3Pressed;
        glm::vec3 input(0.0f);
        float rollInput = 0.0f;

        // =========================
        // KEYBOARD INPUT
        // =========================

        controller.BoostMultiplier = 1.6f; // COD tac sprint seems to add 60%, normal is ~40%

        if (Input::IsKeyPressed(Key::W)) input.z += 1.0f;
        if (Input::IsKeyPressed(Key::S)) input.z -= 1.0f;

        if (Input::IsKeyPressed(Key::D)) input.x += 1.0f;
        if (Input::IsKeyPressed(Key::A)) input.x -= 1.0f;

        if (Input::IsKeyPressed(Key::Space)) input.y += 1.0f;
        if (Input::IsKeyPressed(Key::LeftControl)) input.y -= 1.0f;

        if (Input::IsKeyPressed(Key::Q)) rollInput = -1.0f;
        if (Input::IsKeyPressed(Key::E)) rollInput =  1.0f;

        // =========================
        // CONTROLLER INPUT
        // =========================


        float lx = ControllerInput::GetAxis(pad, GamepadAxis::LeftX);
        float ly = ControllerInput::GetAxis(pad, GamepadAxis::LeftY);

        float rx = ControllerInput::GetAxis(pad, GamepadAxis::RightX);
        float ry = ControllerInput::GetAxis(pad, GamepadAxis::RightY);

        // Movement
        input.x += lx;
        input.z += -ly;

        // Vertical
        float Cross =
            ControllerInput::IsButtonPressed(
                pad,
                GamepadButton::Cross
            );

        float Circle =
            ControllerInput::IsButtonPressed(
                pad,
                GamepadButton::Circle
            );

        input.y += Cross - Circle;

        // =========================
		// LOOK INPUT (FIXED)
		// =========================

		glm::vec2 look(0.0f);

		// Controller
		look.x += rx * 10.0f;
		look.y += ry * 10.0f;

		// Mouse (always include it, don't conditionally overwrite)
		look.x += Input::GetMouseDeltaX();
		look.y += Input::GetMouseDeltaY();

		// Keyboard arrows
		float keyLookSpeed = 540.0f * dt;

		if (Input::IsKeyPressed(Key::Left))
		    look.x -= keyLookSpeed;

		if (Input::IsKeyPressed(Key::Right))
		    look.x += keyLookSpeed;

		if (Input::IsKeyPressed(Key::Up))
		    look.y -= keyLookSpeed*(9.f/16.f);

		if (Input::IsKeyPressed(Key::Down))
		    look.y += keyLookSpeed*(9.f/16.f);

		// FINAL OUTPUT (ONLY ONCE)
		controller.LookDeltaX = look.x;
		controller.LookDeltaY = look.y;


        // Roll
        float LRoll =
            -ControllerInput::GetAxis(
                pad,
                GamepadAxis::L2
            );

        float RRoll =
            ControllerInput::GetAxis(
                pad,
                GamepadAxis::R2
            );

        if (rollInput == 0.0f)
            rollInput = LRoll + RRoll;

        // Boost
        controller.Boost =
            ControllerInput::IsButtonPressed(
                pad,
                GamepadButton::L3
            )
            || Input::IsKeyPressed(Key::LeftShift);

        // Final Inputs
        controller.MoveInput = input;
        controller.RollInput = rollInput;

        //// Mouse fallback if no stick input
        //if (rx == 0.0f && ry == 0.0f)
        //{
        //    controller.LookDeltaX =
        //        Input::GetMouseDeltaX();

        //    controller.LookDeltaY =
        //        Input::GetMouseDeltaY();
        //}

        //WK_CORE_INFO(
        //    "Cross: {0:.3f} | Circle: {1:.3f} | [{2:.3f}, {3:.3f}]",
        //    Cross,
        //    Circle,
        //    LRoll,
        //    RRoll
        //);
    }
}

}
