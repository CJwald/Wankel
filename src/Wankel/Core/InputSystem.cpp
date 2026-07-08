#include "wkpch.h"
#include "InputSystem.h"
#include "Wankel/Core/ControllerInput.h"

#include <SDL3/SDL.h>

namespace Wankel {

static std::vector<SDL_Gamepad*> s_Gamepads;
static bool s_Initialized = false;

bool InputSystem::Init() {
    if (SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS) != 0) {
        WK_CORE_ERROR("SDL Init failed: {0}", SDL_GetError());
        return false;
    }

    s_Initialized = true;

    WK_CORE_INFO("SDL Gamepad subsystem initialized");

    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);

    for (int i = 0; i < count; i++) {
        SDL_Gamepad* pad = SDL_OpenGamepad(ids[i]);

        if (pad) {
            s_Gamepads.push_back(pad);

            WK_CORE_INFO("Controller connected: {0}", SDL_GetGamepadName(pad));
        }
    }

    SDL_free(ids);

    return true;
}

void InputSystem::Shutdown() {
    if (!s_Initialized)
        return;

    for (auto* pad : s_Gamepads) {
        if (pad)
            SDL_CloseGamepad(pad);
    }

    s_Gamepads.clear();

    SDL_Quit();
    s_Initialized = false;
}

void InputSystem::PollControllers() {
    if (!s_Initialized)
        return;

    SDL_UpdateGamepads();

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_EVENT_GAMEPAD_ADDED: {
                SDL_Gamepad* pad = SDL_OpenGamepad(e.gdevice.which);

                if (pad) {
                    s_Gamepads.push_back(pad);

                    WK_CORE_INFO("Controller connected: {0}", SDL_GetGamepadName(pad));
                }

                break;
            }

            case SDL_EVENT_GAMEPAD_REMOVED: {
                SDL_JoystickID removedId = e.gdevice.which;

                auto it = std::find_if(s_Gamepads.begin(), s_Gamepads.end(), [removedId](SDL_Gamepad* pad) {
                    return pad && SDL_GetGamepadID(pad) == removedId;
                });

                if (it != s_Gamepads.end()) {
                    WK_CORE_INFO("Controller disconnected: {0}", SDL_GetGamepadName(*it));
                    SDL_CloseGamepad(*it);
                    s_Gamepads.erase(it);
                }

                break;
            }
        }
    }

    for (size_t i = 0; i < s_Gamepads.size() && i < (size_t)ControllerInput::MaxControllers; i++) {
        SDL_Gamepad* pad = s_Gamepads[i];

        if (!pad)
            continue;

        // AXES
        float lx = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX) / 32767.0f;
        float ly = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY) / 32767.0f;
        float rx = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHTX) / 32767.0f;
        float ry = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHTY) / 32767.0f;

        float l2 = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) / 32767.0f;
        float r2 = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) / 32767.0f;

        ControllerInput::SetAxis((int)i, (int)GamepadAxis::LeftX, lx);
        ControllerInput::SetAxis((int)i, (int)GamepadAxis::LeftY, ly);

        ControllerInput::SetAxis((int)i, (int)GamepadAxis::RightX, rx);
        ControllerInput::SetAxis((int)i, (int)GamepadAxis::RightY, ry);

        ControllerInput::SetAxis((int)i, (int)GamepadAxis::L2, l2);
        ControllerInput::SetAxis((int)i, (int)GamepadAxis::R2, r2);

        // FACE BUTTONS
        ControllerInput::SetButton((int)i, (int)GamepadButton::Cross,
                                   SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_SOUTH));

        ControllerInput::SetButton((int)i, (int)GamepadButton::Circle,
                                   SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_EAST));

        ControllerInput::SetButton((int)i, (int)GamepadButton::Square,
                                   SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_WEST));

        ControllerInput::SetButton((int)i, (int)GamepadButton::Triangle,
                                   SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_NORTH));

        // STICKS
        ControllerInput::SetButton((int)i, (int)GamepadButton::L3,
                                   SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_LEFT_STICK));

        ControllerInput::SetButton((int)i, (int)GamepadButton::R3,
                                   SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_RIGHT_STICK));

        // SHOULDERS
        ControllerInput::SetButton((int)i, (int)GamepadButton::L1,
                                   SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));

        ControllerInput::SetButton((int)i, (int)GamepadButton::R1,
                                   SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER));

        // CENTER BUTTONS
        ControllerInput::SetButton((int)i, (int)GamepadButton::Back,
                                   SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_BACK));

        ControllerInput::SetButton((int)i, (int)GamepadButton::Start,
                                   SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_START));

        ControllerInput::SetButton((int)i, (int)GamepadButton::PS, SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_GUIDE));

        // DPAD
        ControllerInput::SetButton((int)i, (int)GamepadButton::DPadUp,
                                   SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_UP));

        ControllerInput::SetButton((int)i, (int)GamepadButton::DPadDown,
                                   SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_DOWN));

        ControllerInput::SetButton((int)i, (int)GamepadButton::DPadLeft,
                                   SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_LEFT));

        ControllerInput::SetButton((int)i, (int)GamepadButton::DPadRight,
                                   SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT));
    }
}

} // namespace Wankel
