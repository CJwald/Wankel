#pragma once
#include <SDL3/SDL.h>

namespace Wankel {

    enum class GamepadAxis {
        LeftX = 0,
        LeftY,
        RightX,
        RightY,
        L2,
        R2
    };
    
	enum class GamepadButton {
		Cross      = SDL_GAMEPAD_BUTTON_SOUTH,
		Circle     = SDL_GAMEPAD_BUTTON_EAST,
		Square     = SDL_GAMEPAD_BUTTON_WEST,
		Triangle   = SDL_GAMEPAD_BUTTON_NORTH,

		Back       = SDL_GAMEPAD_BUTTON_BACK,
		Start      = SDL_GAMEPAD_BUTTON_START,
		PS         = SDL_GAMEPAD_BUTTON_GUIDE,

		L3         = SDL_GAMEPAD_BUTTON_LEFT_STICK,
		R3         = SDL_GAMEPAD_BUTTON_RIGHT_STICK,

		L1         = SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
		R1         = SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,

		DPadUp     = SDL_GAMEPAD_BUTTON_DPAD_UP,
		DPadDown   = SDL_GAMEPAD_BUTTON_DPAD_DOWN,
		DPadLeft   = SDL_GAMEPAD_BUTTON_DPAD_LEFT,
		DPadRight  = SDL_GAMEPAD_BUTTON_DPAD_RIGHT
	};

}
