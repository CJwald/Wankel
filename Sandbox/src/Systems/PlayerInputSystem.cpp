#include "PlayerInputSystem.h"

#include <Wankel/Core/Input.h>
#include <Wankel/Core/ControllerInput.h>
#include <Wankel/Core/KeyCodes.h>
#include <Wankel/ECS/Components.h>
#include <Wankel/Core/Log.h>

namespace Wankel {

void PlayerInputSystem::Update(Scene& scene, float dt, bool gameFocused) {
    auto controllerView = scene.Registry().view<PlayerControllerComponent>();

    // TODO: these probably shouldnt live here in the full game
    constexpr float MouseSensitivity = 1.0f;     // pixels -> degrees multiplier
    constexpr float StickTurnSpeed   = 720.0f;    // deg/sec at full tilt
    constexpr float keyTurnSpeed     = 540.0f;    // deg/sec
    
    for (auto entity : controllerView) {

        // Skip if game not focused
        if (!gameFocused)
            continue;

        auto& controller = controllerView.get<PlayerControllerComponent>(entity);

        int pad = 0;
        
		// LOOK MODE TOGGLE (R3)
		static bool r3PressedLastFrame = false;
		bool r3Pressed = false;

        if (Input::IsKeyPressed(Key::X) || ControllerInput::IsButtonPressed(pad, GamepadButton::R3)) {
			r3Pressed = true;
		}

		if (r3Pressed && !r3PressedLastFrame) {
			if (controller.Mode == PlayerControllerComponent::LookMode::FPS) {
				controller.Mode = PlayerControllerComponent::LookMode::Flight;
			}
			else {
				controller.Mode = PlayerControllerComponent::LookMode::FPS;
			}
		}

		r3PressedLastFrame = r3Pressed;
        glm::vec3 input(0.0f);
        float rollInput = 0.0f;

        // KEYBOARD INPUT
        controller.BoostMultiplier = 1.6f; // COD tac sprint seems to add 60%, normal is ~40%

        if (Input::IsKeyPressed(Key::W)) input.z += 1.0f;
        if (Input::IsKeyPressed(Key::S)) input.z -= 1.0f;

        if (Input::IsKeyPressed(Key::D)) input.x += 1.0f;
        if (Input::IsKeyPressed(Key::A)) input.x -= 1.0f;

        if (Input::IsKeyPressed(Key::Space)) input.y += 1.0f;
        if (Input::IsKeyPressed(Key::LeftControl)) input.y -= 1.0f;

        if (Input::IsKeyPressed(Key::Q)) rollInput = -1.0f;
        if (Input::IsKeyPressed(Key::E)) rollInput =  1.0f;

        // CONTROLLER INPUT
        float lx = ControllerInput::GetAxis(pad, GamepadAxis::LeftX);
        float ly = ControllerInput::GetAxis(pad, GamepadAxis::LeftY);

        float rx = ControllerInput::GetAxis(pad, GamepadAxis::RightX);
        float ry = ControllerInput::GetAxis(pad, GamepadAxis::RightY);

        // Movement
        input.x += lx;
        input.z += -ly;

        // Vertical
        float Cross =  ControllerInput::IsButtonPressed( pad, GamepadButton::Cross );
        float Circle = ControllerInput::IsButtonPressed( pad, GamepadButton::Circle );
        input.y += Cross - Circle;

		// LOOK INPUT
		glm::vec2 lookVelocity(0.0f);

		// Controller
		lookVelocity.x += rx * StickTurnSpeed;
		lookVelocity.y += ry * StickTurnSpeed;

		// Mouse (always include it, don't conditionally overwrite)
		lookVelocity.x += Input::GetMouseDeltaX() * MouseSensitivity / dt;
		lookVelocity.y += Input::GetMouseDeltaY() * MouseSensitivity / dt;

		// Keyboard arrows
		if (Input::IsKeyPressed(Key::Left))
		    lookVelocity.x -= keyTurnSpeed;

		if (Input::IsKeyPressed(Key::Right))
		    lookVelocity.x += keyTurnSpeed;

		if (Input::IsKeyPressed(Key::Up))
		    lookVelocity.y -= keyTurnSpeed*(9.f/16.f); // scale testing

		if (Input::IsKeyPressed(Key::Down))
		    lookVelocity.y += keyTurnSpeed*(9.f/16.f); // scale testing

		// FINAL OUTPUT
		controller.LookDeltaX = lookVelocity.x * dt;
		controller.LookDeltaY = lookVelocity.y * dt;


        // Roll
        float LRoll = -ControllerInput::GetAxis( pad, GamepadAxis::L2 );
        float RRoll =  ControllerInput::GetAxis( pad, GamepadAxis::R2 );
        if (rollInput == 0.0f)
            rollInput = LRoll + RRoll;

        // Boost
        controller.Boost = ControllerInput::IsButtonPressed( pad, GamepadButton::L3 )
            || Input::IsKeyPressed(Key::LeftShift);

        // Final Inputs
        controller.MoveInput = input;
        controller.RollInput = rollInput;
    }
}

}
