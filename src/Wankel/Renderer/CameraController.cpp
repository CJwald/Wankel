#include "wkpch.h"
#include "Wankel/Renderer/CameraController.h"
#include "Wankel/Core/Input.h"
#include "Wankel/Core/KeyCodes.h"
#include "Wankel/Core/Log.h"

#include <glm/gtx/quaternion.hpp>

namespace Wankel {

	CameraController::CameraController(float aspect)
		: m_Camera(45.0f, aspect, 0.1f, 100.0f)
	{}

	void CameraController::OnUpdate(float dt) {
	    glm::vec3 position = m_Camera.GetPosition();

	    // Use current orientation for movement directions
	    glm::vec3 forward = m_Camera.GetForward();
	    glm::vec3 right   = m_Camera.GetRight();
	    glm::vec3 up      = m_Camera.GetUp();

	    float velocity = m_MoveSpeed * dt;

	    // ========================
	    // Keyboard movement
	    // ========================
	    if (Input::IsKeyPressed(Key::W)) position += forward * velocity;
	    if (Input::IsKeyPressed(Key::S)) position -= forward * velocity;
	    if (Input::IsKeyPressed(Key::D)) position += right * velocity;
	    if (Input::IsKeyPressed(Key::A)) position -= right * velocity;
	    if (Input::IsKeyPressed(Key::Space)) position += up * velocity;
	    if (Input::IsKeyPressed(Key::LeftControl)) position -= up * velocity;

	    // ========================
	    // Mouse look
	    // ========================
	    float dx = Input::GetMouseDeltaX() * m_WindowSensitivity;
	    float dy = Input::GetMouseDeltaY() * m_WindowSensitivity;

	    if (dx != 0.0f || dy != 0.0f) {
	        // --- YAW (world up, always stable) ---
	        glm::quat yaw = glm::angleAxis(
	            -dx * m_RotationSpeed,
	            glm::vec3(0.0f, 1.0f, 0.0f)
	        );
	        m_Orientation = glm::normalize(yaw * m_Orientation);

	        // --- recompute right AFTER yaw ---
	        glm::vec3 newRight = m_Orientation * glm::vec3(1, 0, 0);

	        // --- PITCH (local right, stable now) ---
	        glm::quat pitch = glm::angleAxis(
	            -dy * m_RotationSpeed,
	            newRight
	        );
	        m_Orientation = glm::normalize(pitch * m_Orientation);
	    }

	    // ========================
	    // Roll (local forward)
	    // ========================
	    glm::vec3 currentForward = m_Orientation * glm::vec3(0, 0, -1);

	    if (Input::IsKeyPressed(Key::E))
	        m_Orientation = glm::normalize(
	            glm::angleAxis(m_RollSpeed * dt, currentForward) * m_Orientation
	        );

	    if (Input::IsKeyPressed(Key::Q))
	        m_Orientation = glm::normalize(
	            glm::angleAxis(-m_RollSpeed * dt, currentForward) * m_Orientation
	        );

	    // ========================
	    // Apply to camera
	    // ========================
	    m_Camera.SetPosition(position);
	    m_Camera.SetOrientation(m_Orientation);
	}

	void CameraController::OnResize(float width, float height) {
	    m_Camera.SetAspect(width / height);
	}
}
