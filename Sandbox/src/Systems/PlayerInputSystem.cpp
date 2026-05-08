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

        glm::vec3 input(0.0f);
        float rollInput = 0.0f;

        // =========================
        // KEYBOARD INPUT
        // =========================

        controller.BoostMultiplier = 2.0f;

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

        int pad = 0;

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

        // Look
        controller.LookDeltaX = rx * 10.0f;
        controller.LookDeltaY = ry * 10.0f;

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

        // Mouse fallback if no stick input
        if (rx == 0.0f && ry == 0.0f)
        {
            controller.LookDeltaX =
                Input::GetMouseDeltaX();

            controller.LookDeltaY =
                Input::GetMouseDeltaY();
        }

        WK_CORE_INFO(
            "Cross: {0:.3f} | Circle: {1:.3f} | [{2:.3f}, {3:.3f}]",
            Cross,
            Circle,
            LRoll,
            RRoll
        );
    }
}

}
