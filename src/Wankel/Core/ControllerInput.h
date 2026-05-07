#pragma once

#include "GamepadCodes.h"

namespace Wankel {

    class ControllerInput {
    public:
        static constexpr int MaxControllers = 4;
        static constexpr int MaxAxes = 8;
        static constexpr int MaxButtons = 32;

        // =========================
        // GAMEPLAY API
        // =========================

        static float GetAxis(int controller, GamepadAxis axis);

        static bool IsButtonPressed(
            int controller,
            GamepadButton button
        );

        // =========================
        // INTERNAL ENGINE API
        // =========================

        static void SetAxis(
            int controller,
            int axis,
            float value
        );

        static void SetButton(
            int controller,
            int button,
            bool pressed
        );

    private:
        static void ApplyDeadzone(float& value, float deadzone = 0.05f);

    private:
        static float s_Axes[MaxControllers][MaxAxes];
        static bool  s_Buttons[MaxControllers][MaxButtons];
    };

}
