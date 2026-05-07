#pragma once

#include <array>

namespace Wankel {

    class ControllerInput {
    public:
        static constexpr int MaxControllers = 4;
        static constexpr int MaxAxes = 8;
        static constexpr int MaxButtons = 32;

        // -----------------------------
        // Query
        // -----------------------------
        static float GetAxis(int controller, int axis);
        static bool IsButtonPressed(int controller, int button);

        // -----------------------------
        // Internal (called by platform/event system)
        // -----------------------------
        static void SetAxis(int controller, int axis, float value);
        static void SetButton(int controller, int button, bool pressed);

        static void ApplyDeadzone(float& value, float deadzone = 0.05f);

    private:
        static float s_Axes[MaxControllers][MaxAxes];
        static bool  s_Buttons[MaxControllers][MaxButtons];
    };

}
