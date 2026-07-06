#include "wkpch.h"
#include "ControllerInput.h"

#include <cmath>

namespace Wankel {

    float ControllerInput::s_Axes[MaxControllers][MaxAxes] = {};
    bool  ControllerInput::s_Buttons[MaxControllers][MaxButtons] = {};

    float ControllerInput::GetAxis( int controller, GamepadAxis axis ) {
        int axisIdx = (int)axis;

        if (controller < 0 || controller >= MaxControllers || axisIdx < 0 || axisIdx >= MaxAxes)
            return 0.0f;

        float v = s_Axes[controller][axisIdx];
        ApplyDeadzone(v);

        return v;
    }

    bool ControllerInput::IsButtonPressed( int controller, GamepadButton button ) {
        int buttonIdx = (int)button;

        if (controller < 0 || controller >= MaxControllers || buttonIdx < 0 || buttonIdx >= MaxButtons)
            return false;

        return s_Buttons[controller][buttonIdx];
    }

    void ControllerInput::SetAxis( int controller, int axis, float value ) {
        if (controller < 0 || controller >= MaxControllers || axis < 0 || axis >= MaxAxes)
            return;

        s_Axes[controller][axis] = value;
    }

    void ControllerInput::SetButton( int controller, int button, bool pressed ) {
        if (controller < 0 || controller >= MaxControllers || button < 0 || button >= MaxButtons)
            return;

        s_Buttons[controller][button] = pressed;
    }

    void ControllerInput::ApplyDeadzone( float& value, float deadzone ) {
        if (std::abs(value) < deadzone)
            value = 0.0f;
    }

}
