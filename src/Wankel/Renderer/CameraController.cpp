#include "Wankel/Renderer/CameraController.h"
#include "Wankel/Core/Input.h"
#include "Wankel/Core/KeyCodes.h"

#include <glm/gtx/quaternion.hpp>
#include "Wankel/Core/Log.h"

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
	
	    // Keyboard movement (should still work)
	    if (Input::IsKeyPressed(Key::W)) position += forward * velocity;
	    if (Input::IsKeyPressed(Key::S)) position -= forward * velocity;
	    if (Input::IsKeyPressed(Key::D)) position += right * velocity;
	    if (Input::IsKeyPressed(Key::A)) position -= right * velocity;
	    if (Input::IsKeyPressed(Key::E)) position += up * velocity;
	    if (Input::IsKeyPressed(Key::Q)) position -= up * velocity;
	
	    // === MOUSE LOOK - with debug ===
	    float dx = Input::GetMouseDeltaX();
	    float dy = Input::GetMouseDeltaY();

		WK_CLIENT_INFO("Mouse Delta this frame: dx={0:.3f}, dy={1:.3f}", dx, dy);

	    if (dx != 0.0f || dy != 0.0f)
	    {
	        glm::quat yaw   = glm::angleAxis(-dx * m_RotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
	        glm::quat pitch = glm::angleAxis(-dy * m_RotationSpeed, right);
	
	        m_Orientation = glm::normalize(yaw * pitch * m_Orientation);
	    }
	
	    // Roll
	    if (Input::IsKeyPressed(Key::Z))
	        m_Orientation = glm::normalize(glm::angleAxis( m_RollSpeed * dt, forward) * m_Orientation);
	    if (Input::IsKeyPressed(Key::C))
	        m_Orientation = glm::normalize(glm::angleAxis(-m_RollSpeed * dt, forward) * m_Orientation);
	
	    m_Camera.SetPosition(position);
	    m_Camera.SetOrientation(m_Orientation);
	
	    // Reset at the VERY END
	    Input::ResetMouseDelta();
	}
}
