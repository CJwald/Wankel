#include "wkpch.h"
#include "InputSystem.h"
#include "Wankel/Core/ControllerInput.h"

#include <SDL3/SDL.h>

namespace Wankel {

    // Store opened controllers
    static std::vector<SDL_Gamepad*> s_Gamepads;

    void InputSystem::Init() {
        // CONTROLLER-ONLY INIT (no video)
        if (SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS) != 0) {
            WK_CORE_ERROR("SDL Init failed: {0}", SDL_GetError());
            return;
        }

        WK_CORE_INFO("SDL Gamepad subsystem initialized");

        // SDL3 uses SDL_JoystickID, NOT SDL_GamepadID
        int count = 0;
        SDL_JoystickID* ids = SDL_GetGamepads(&count);

        for (int i = 0; i < count; i++) {
            SDL_Gamepad* pad = SDL_OpenGamepad(ids[i]);
            if (pad) { 
				// AXES
				// Left
    			ControllerInput::SetAxis( (int)i, (int)GamepadAxis::LeftX,
    			    SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX) / 32767.0f );
    			ControllerInput::SetAxis( (int)i, (int)GamepadAxis::LeftY,
    			    SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY) / 32767.0f );

				// Right
    			ControllerInput::SetAxis( (int)i, (int)GamepadAxis::RightX,
    			    SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHTX) / 32767.0f	);
    			ControllerInput::SetAxis( (int)i, (int)GamepadAxis::RightY,
    			    SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHTY) / 32767.0f );

				// Triggers
    			ControllerInput::SetAxis( (int)i, (int)GamepadAxis::L2,
    			    SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) / 32767.0f );
    			ControllerInput::SetAxis( (int)i, (int)GamepadAxis::R2, 
					SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) / 32767.0f );

    			// BUTTONS
    			for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++) {
    			    bool pressed = SDL_GetGamepadButton( pad, (SDL_GamepadButton)b );
    			    ControllerInput::SetButton( (int)i, b, pressed );
    			}
            }
        }

        SDL_free(ids);
    }

    void InputSystem::Shutdown() {
        for (auto* pad : s_Gamepads) {
            SDL_CloseGamepad(pad);
        }
        s_Gamepads.clear();
        SDL_Quit();
    }

    void InputSystem::PollControllers()
    {
        // Update controller state
        SDL_UpdateGamepads();

        // Minimal event handling (NO window usage)
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
                    WK_CORE_INFO("Controller disconnected");
                    // TODO: properly remove from vector later
                    break;
                }
            }
        }

        // Poll all controllers
        for (size_t i = 0; i < s_Gamepads.size(); i++) {
            SDL_Gamepad* pad = s_Gamepads[i];
            if (!pad) continue;

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
            
            ControllerInput::SetButton( (int)i, (int)GamepadButton::Cross,
			    SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_SOUTH) );
			
			ControllerInput::SetButton( (int)i, (int)GamepadButton::Circle,
			    SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_EAST) );
			
			ControllerInput::SetButton( (int)i, (int)GamepadButton::Square,
			    SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_WEST) );
			
			ControllerInput::SetButton( (int)i, (int)GamepadButton::Triangle,
			    SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_NORTH) );
			
			ControllerInput::SetButton( (int)i, (int)GamepadButton::L3,
			    SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_LEFT_STICK) );
			
			ControllerInput::SetButton( (int)i, (int)GamepadButton::R3,
			    SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_RIGHT_STICK) );
        }
    }
}
