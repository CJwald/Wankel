#pragma once

#include "Wankel/Core/KeyCodes.h"
#include "Wankel/Core/MouseCodes.h"

namespace Wankel {

	class Input {
	public:
		// -----------------------------
		// Keyboard
		// -----------------------------
		static bool IsKeyPressed(KeyCode key);

		// -----------------------------
		// Mouse buttons
		// -----------------------------
		static bool IsMouseButtonPressed(MouseCode button);

		// -----------------------------
		// Mouse movement (DELTA ONLY)
		// -----------------------------
		static float GetMouseDeltaX();
		static float GetMouseDeltaY();

		// Called by platform layer (DO NOT call manually)
		static void SetMouseDelta(float dx, float dy);

	private:
		static float s_MouseDeltaX;
		static float s_MouseDeltaY;
	};

}
