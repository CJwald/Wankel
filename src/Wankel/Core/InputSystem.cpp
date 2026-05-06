#include "wkpch.h"
#include "InputSystem.h"

#include <SDL3/SDL.h>

#include "Wankel/Core/Application.h"
#include "Wankel/Core/Events/ControllerAxisEvent.h"
#include "Wankel/Core/Events/ControllerButtonEvent.h"

namespace Wankel {

    void InputSystem::Init() {
        if (SDL_Init(SDL_INIT_GAMEPAD) != 0) {
            WK_CORE_ERROR("SDL Init failed: {0}", SDL_GetError());
            return;
        }

        WK_CORE_INFO("SDL Gamepad subsystem initialized");
    }

    void InputSystem::Shutdown() {
        SDL_Quit();
    }

    void InputSystem::PollEvents() {
        SDL_Event e;

        while (SDL_PollEvent(&e)) {

            switch (e.type) {

                case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                {
                    ControllerAxisMovedEvent ev(
                        e.gaxis.which,
                        e.gaxis.axis,
                        e.gaxis.value / 32767.0f
                    );
                    Application::Get().OnEvent(ev);
                    break;
                }

                case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                {
                    ControllerButtonPressedEvent ev(
                        e.gbutton.which,
                        e.gbutton.button
                    );
                    Application::Get().OnEvent(ev);
                    break;
                }

                case SDL_EVENT_GAMEPAD_BUTTON_UP:
                {
                    ControllerButtonReleasedEvent ev(
                        e.gbutton.which,
                        e.gbutton.button
                    );
                    Application::Get().OnEvent(ev);
                    break;
                }
                
                // =========================
		        // CONTROLLER ADDED
		        // =========================
		        case SDL_EVENT_GAMEPAD_ADDED:
		        {
		            SDL_Gamepad* pad = SDL_OpenGamepad(e.gdevice.which);

		            if (pad) {
		                WK_CORE_INFO(
		                    "Controller connected: {0}",
		                    SDL_GetGamepadName(pad)
		                );
		            }
		            break;
		        }

		        // =========================
		        // CONTROLLER REMOVED
		        // =========================
		        case SDL_EVENT_GAMEPAD_REMOVED:
		        {
		            WK_CORE_INFO("Controller disconnected");
		            break;
		        }
            }
        }
    }

}
