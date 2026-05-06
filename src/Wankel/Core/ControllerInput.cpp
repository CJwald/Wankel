#include "wkpch.h"
#include "ControllerInput.h"
#include <cmath>

namespace Wankel {

    float ControllerInput::s_Axes[MaxControllers][MaxAxes] = {};
    bool  ControllerInput::s_Buttons[MaxControllers][MaxButtons] = {};

    float ControllerInput::GetAxis(int controller, int axis) {
        float v = s_Axes[controller][axis];
        ApplyDeadzone(v);
        return v;
    }

    bool ControllerInput::IsButtonPressed(int controller, int button) {
        return s_Buttons[controller][button];
    }

    void ControllerInput::SetAxis(int controller, int axis, float value) {
        s_Axes[controller][axis] = value;
    }

    void ControllerInput::SetButton(int controller, int button, bool pressed) {
        s_Buttons[controller][button] = pressed;
    }

    void ControllerInput::ApplyDeadzone(float& value, float deadzone) {
        if (std::abs(value) < deadzone)
            value = 0.0f;
    }

}
