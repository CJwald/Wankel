#include "wkpch.h"
#include "Wankel/Core/Input.h"
#include "Wankel/Core/Application.h"

#include <GLFW/glfw3.h>

namespace Wankel {

// Static storage
float Input::s_MouseDeltaX = 0.0f;
float Input::s_MouseDeltaY = 0.0f;
float Input::s_MouseX = 0.0f;
float Input::s_MouseY = 0.0f;

// Keyboard
bool Input::IsKeyPressed(KeyCode key) {
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    return glfwGetKey(window, (int)key) == GLFW_PRESS;
}

// Mouse buttons
bool Input::IsMouseButtonPressed(MouseCode button) {
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    return glfwGetMouseButton(window, (int)button) == GLFW_PRESS;
}

float Input::GetMouseDeltaX() {
    return s_MouseDeltaX;
}

float Input::GetMouseDeltaY() {
    return s_MouseDeltaY;
}

void Input::SetMouseDelta(float dx, float dy) {
    // Accumulate, don't overwrite - glfwPollEvents() can dispatch the cursor-pos callback multiple
    // times per poll (multiple raw mouse-move events queued since the last poll, common on a slower
    // frame), and overwriting kept only the last of those, silently dropping the rest. That made
    // look-sensitivity scale inversely with frame time: heavier scenes (more queued events lost)
    // felt sluggish, lighter ones (fewer/no events coalesced, e.g. the ~500fps Void) felt "full
    // speed" - the same absolute mouse motion should always add up to the same rotation regardless
    // of how many frames or callback invocations it arrived over.
    s_MouseDeltaX += dx;
    s_MouseDeltaY += dy;
}

// Reset every frame after use
void Input::ResetMouseDelta() {
    s_MouseDeltaX = 0.0f;
    s_MouseDeltaY = 0.0f;
}

float Input::GetMouseX() {
    return s_MouseX;
}

float Input::GetMouseY() {
    return s_MouseY;
}

void Input::SetMousePosition(float x, float y) {
    s_MouseX = x;
    s_MouseY = y;
}

} // namespace Wankel
