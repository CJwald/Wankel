#pragma once

#include "Wankel/Core/KeyCodes.h"
#include "Wankel/Core/MouseCodes.h"

namespace Wankel {

class Input {
public:
    // Keyboard
    static bool IsKeyPressed(KeyCode key);

    // Mouse buttons
    static bool IsMouseButtonPressed(MouseCode button);

    // Mouse movement (DELTA ONLY)
    static float GetMouseDeltaX();
    static float GetMouseDeltaY();

    // Called by platform layer
    static void SetMouseDelta(float dx, float dy);

    // Reset delta at the end of the frame (prevents sticking)
    static void ResetMouseDelta();

    // Absolute cursor position (window-content-area pixels, origin top-left) - for UI hit-testing
    // (e.g. menu button hover/click), not gameplay look (use the delta API above for that).
    static float GetMouseX();
    static float GetMouseY();

    // Called by platform layer
    static void SetMousePosition(float x, float y);

private:
    static float s_MouseDeltaX;
    static float s_MouseDeltaY;
    static float s_MouseX;
    static float s_MouseY;
};

} // namespace Wankel
