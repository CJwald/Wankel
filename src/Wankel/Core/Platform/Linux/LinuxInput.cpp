#include "wkpch.h"
#include "Wankel/Core/Input.h"
#include "Wankel/Core/Application.h"

#include <GLFW/glfw3.h>

namespace Wankel {

	// -----------------------------
	// Static storage
	// -----------------------------
	float Input::s_MouseDeltaX = 0.0f;
	float Input::s_MouseDeltaY = 0.0f;

	// -----------------------------
	// Keyboard
	// -----------------------------
	bool Input::IsKeyPressed(KeyCode key) {
		auto window = static_cast<GLFWwindow*>(
			Application::Get().GetWindow().GetNativeWindow());

		return glfwGetKey(window, (int)key) == GLFW_PRESS;
	}

	// -----------------------------
	// Mouse buttons
	// -----------------------------
	bool Input::IsMouseButtonPressed(MouseCode button) {
		auto window = static_cast<GLFWwindow*>(
			Application::Get().GetWindow().GetNativeWindow());

		return glfwGetMouseButton(window, (int)button) == GLFW_PRESS;
	}

	// -----------------------------
	// Mouse delta (EVENT DRIVEN)
	// -----------------------------
	float Input::GetMouseDeltaX() { return s_MouseDeltaX; }
	float Input::GetMouseDeltaY() { return s_MouseDeltaY; }

	void Input::SetMouseDelta(float dx, float dy) {
		s_MouseDeltaX = dx;
		s_MouseDeltaY = dy;
	}

	// Reset every frame after use
	void Input::ResetMouseDelta() {
		s_MouseDeltaX = 0.0f;
		s_MouseDeltaY = 0.0f;
	}

}
