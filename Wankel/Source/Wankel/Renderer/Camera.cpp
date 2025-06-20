#include "Camera.h"
//#include "Response.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "../Input/Input.h" // TODO, remove all inputs from this 

//#include "SDL.h"

using namespace Walnut;

Camera::Camera(float verticalFOV, float nearClip, float farClip)
	: m_VerticalFOV(verticalFOV), m_NearClip(nearClip), m_FarClip(farClip) {
	m_ForwardDirection = glm::vec3(-1, 0.0, 0.0);
	m_UpDirection = glm::vec3(0.0, 1.0, 0.0);
	m_RightDirection = glm::vec3(0.0, 0.0, -1.0);
	m_Position = glm::vec3(20, 0, 0);
}

bool Camera::OnUpdate(float ts) {
	glm::vec2 mousePos = Input::GetMousePosition();
	glm::vec2 delta = (mousePos - m_LastMousePosition) * 0.002f;
	m_LastMousePosition = mousePos;

	//if (!Input::IsMouseButtonDown(MouseButton::Right)) {
	//	Input::SetCursorMode(CursorMode::Normal);
	//	return false;
	//}
	//Input::SetCursorMode(CursorMode::Locked);

	// Shift camera to other end of border if it passes the border
	float borderRadius = 100.0f; // TODO: needs to be dependent on the scene border sphere
	if (glm::length(m_Position) >= borderRadius)
		m_Position = m_Position - 2.0f * m_Position;

	if (Input::IsKeyDown(KeyCode::Escape)) { // toggle cursor mode
		if (m_Cursor) {
			m_Cursor = false;
		} else {
			m_Cursor = true;
		}
	}
	if (m_Cursor) {
		Input::SetCursorMode(CursorMode::Normal);
		return false;
	} else {
		Input::SetCursorMode(CursorMode::Locked);
	}

	

	bool moved = false;

	m_RightDirection = glm::cross(m_ForwardDirection, m_UpDirection);

	float speed = 50.0f;// 8.5f; // m/s
	float rotSensitivity = 1.2; 

	// Movement
	if (Input::IsKeyDown(KeyCode::W)) {                // FORWARD
		m_Position += m_ForwardDirection * speed * ts;
		moved = true;
	} else if (Input::IsKeyDown(KeyCode::S)) {         // BACKWARD
		m_Position -= m_ForwardDirection * speed * ts;
		moved = true;
	}
	if (Input::IsKeyDown(KeyCode::A)) {                // LEFT
		m_Position -= m_RightDirection * speed * ts;
		moved = true;
	} else if (Input::IsKeyDown(KeyCode::D)) {         // RIGHT
		m_Position += m_RightDirection * speed * ts;
		moved = true;
	}
	if (Input::IsKeyDown(KeyCode::LeftControl)) {                // DOWN
		m_Position -= m_UpDirection * speed * ts;
		moved = true;
	} else if (Input::IsKeyDown(KeyCode::Space)) {     // UP
		m_Position += m_UpDirection * speed * ts;
		moved = true;
	} else if (Input::IsKeyDown(KeyCode::E)) {         // ROLL RIGHT
		float rollDelta = ts * -GetRollSpeed() * rotSensitivity;
		glm::quat q = glm::normalize(glm::angleAxis(-rollDelta, m_ForwardDirection));
		m_UpDirection = glm::rotate(q, m_UpDirection);
		m_RightDirection = glm::rotate(q, m_RightDirection);
		moved = true;
	} else if (Input::IsKeyDown(KeyCode::Q)) {         // ROLL LEFT
		float rollDelta = ts * GetRollSpeed() * rotSensitivity;
		glm::quat q = glm::normalize(glm::angleAxis(-rollDelta, m_ForwardDirection));
		m_UpDirection = glm::rotate(q, m_UpDirection);
		m_RightDirection = glm::rotate(q, m_RightDirection);
		moved = true;
	}

	// Rotation
	if (delta.x != 0.0f || delta.y != 0.0f) {
		float pitchDelta = delta.y * GetRotationSpeed() * rotSensitivity;
		float yawDelta = delta.x * GetRotationSpeed() * rotSensitivity;

		glm::quat q = glm::normalize(glm::cross(glm::angleAxis(-pitchDelta, m_RightDirection),
			glm::angleAxis(-yawDelta, m_UpDirection)));
		m_ForwardDirection = glm::rotate(q, m_ForwardDirection);

		moved = true;
	}

	if (moved) {
		RecalculateView();
		RecalculateRayDirections();
	}

	return moved;
}

void Camera::OnResize(uint32_t width, uint32_t height) {
	if (width == m_ViewportWidth && height == m_ViewportHeight)
		return;

	m_ViewportWidth = width;
	m_ViewportHeight = height;

	RecalculateProjection();
	RecalculateRayDirections();
}

float Camera::GetRotationSpeed() {
	return 1.0f;
}

float Camera::GetRollSpeed() {
	return 2.0f;
}

void Camera::RecalculateProjection() {
	m_Projection = glm::perspectiveFov(glm::radians(m_VerticalFOV), (float)m_ViewportWidth, (float)m_ViewportHeight, m_NearClip, m_FarClip);
	m_InverseProjection = glm::inverse(m_Projection);
}

void Camera::RecalculateView() {
	m_View = glm::lookAt(m_Position, m_Position + m_ForwardDirection, m_UpDirection);
	m_InverseView = glm::inverse(m_View);
}

void Camera::RecalculateRayDirections() {
	m_RayDirections.resize(m_ViewportWidth * m_ViewportHeight);

	for (uint32_t y = 0; y < m_ViewportHeight; y++)
	{
		for (uint32_t x = 0; x < m_ViewportWidth; x++)
		{
			glm::vec2 coord = { (float)x / (float)m_ViewportWidth, (float)y / (float)m_ViewportHeight };
			coord = coord * 2.0f - 1.0f; // -1 -> 1

			glm::vec4 target = m_InverseProjection * glm::vec4(coord.x, coord.y, 1, 1);
			glm::vec3 rayDirection = glm::vec3(m_InverseView * glm::vec4(glm::normalize(glm::vec3(target) / target.w), 0)); // World space
			m_RayDirections[x + y * m_ViewportWidth] = rayDirection;
		}
	}
}
