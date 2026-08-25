#include "wkpch.h"
#include "InputSystem.h"
#include "Wankel/Core/ControllerInput.h"

#include <SDL3/SDL.h>

namespace Wankel {

static std::vector<SDL_Gamepad*> s_Gamepads;
static bool s_Initialized = false;

bool InputSystem::Init() {
    // SDL3's SDL_Init returns bool (true = success), not SDL2's old "0 = success" int convention -
    // this was inverted, so every successful init was logged and treated as a failure, permanently
    // disabling gamepad support on every run regardless of whether a controller was even connected.
    if (!SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS)) {
        WK_CORE_ERROR("SDL Init failed: {0}", SDL_GetError());
        return false;
    }

    s_Initialized = true;

    WK_CORE_INFO("SDL Gamepad subsystem initialized");

    // Deliberately NOT pre-scanning/opening already-connected gamepads here (via SDL_GetGamepads +
    // SDL_OpenGamepad) - SDL itself synthesizes an SDL_EVENT_GAMEPAD_ADDED for every gamepad already
    // plugged in as soon as the event queue is first pumped (see SDL_events.h's SDL_GamepadDeviceEvent
    // comment), which the very first PollControllers() call below will pick up via the exact same
    // ADDED-event path used for a live hotplug. Doing both used to double-open every controller that
    // was already connected at launch (one handle from this scan, a second from the replayed ADDED
    // event) - s_Gamepads held two entries for one physical pad from frame one, so a later real
    // unplug/replug would land the reconnected device at index 1 while gameplay code (hardcoded to
    // pad index 0) kept reading the stale leftover handle at index 0, making the controller look dead
    // after a reconnect.
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
                SDL_JoystickID addedId = e.gdevice.which;

                // Defends against ever double-registering one physical pad (e.g. a duplicate ADDED
                // event from a flaky driver) the same way the removed startup pre-scan used to -
                // s_Gamepads must stay a 1:1 map of instance ID -> slot for the hardcoded pad-0
                // assumption gameplay code makes to hold.
                bool alreadyTracked = std::any_of(s_Gamepads.begin(), s_Gamepads.end(), [addedId](SDL_Gamepad* pad) {
                    return pad && SDL_GetGamepadID(pad) == addedId;
                });

                if (!alreadyTracked) {
                    SDL_Gamepad* pad = SDL_OpenGamepad(addedId);

                    if (pad) {
                        s_Gamepads.push_back(pad);

                        WK_CORE_INFO("Controller connected: {0}", SDL_GetGamepadName(pad));
                    }
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

        ControllerInput::SetButton((int)i, (int)GamepadButton::Touchpad,
                                   SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_TOUCHPAD));

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
