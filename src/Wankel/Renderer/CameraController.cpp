#include "Wankel/Renderer/CameraController.h"
#include "Wankel/Core/Input.h"
#include "Wankel/Core/KeyCodes.h"

#include <glm/gtx/quaternion.hpp>

namespace Wankel {

	CameraController::CameraController(float aspect)
		: m_Camera(45.0f, aspect, 0.1f, 100.0f)
	{}

	void CameraController::OnUpdate(float dt)
	{
		glm::vec3 position = m_Camera.GetPosition();

		glm::vec3 forward = m_Camera.GetForward();
		glm::vec3 right   = m_Camera.GetRight();
		glm::vec3 up      = m_Camera.GetUp();

		float velocity = m_MoveSpeed * dt;

		// -----------------------------
		// Movement (using your KeyCodes)
		// -----------------------------
		if (Input::IsKeyPressed(Key::W)) position += forward * velocity;
		if (Input::IsKeyPressed(Key::S)) position -= forward * velocity;

		if (Input::IsKeyPressed(Key::D)) position += right * velocity;
		if (Input::IsKeyPressed(Key::A)) position -= right * velocity;

		if (Input::IsKeyPressed(Key::E)) position += up * velocity;
		if (Input::IsKeyPressed(Key::Q)) position -= up * velocity;

		// -----------------------------
		// Mouse rotation
		// -----------------------------
		float dx = Input::GetMouseDeltaX() * m_RotationSpeed;
		float dy = Input::GetMouseDeltaY() * m_RotationSpeed;

		glm::quat yaw   = glm::angleAxis(-dx, glm::vec3(0,1,0));
		glm::quat pitch = glm::angleAxis(-dy, right);

		m_Orientation = glm::normalize(yaw * pitch * m_Orientation);

		// -----------------------------
		// Roll
		// -----------------------------
		if (Input::IsKeyPressed(Key::Z))
			m_Orientation = glm::normalize(
				glm::angleAxis(1.5f * dt, forward) * m_Orientation);

		if (Input::IsKeyPressed(Key::C))
			m_Orientation = glm::normalize(
				glm::angleAxis(-1.5f * dt, forward) * m_Orientation);

		m_Camera.SetPosition(position);
		m_Camera.SetOrientation(m_Orientation);
	}

}
