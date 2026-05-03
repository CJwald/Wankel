#pragma once

#include "Camera.h"

namespace Wankel {

	class CameraController {
	public:
		CameraController(float aspect);

		void OnUpdate(float deltaTime);
		void OnResize(float width, float height);

		Camera& GetCamera() { return m_Camera; }

	private:
		Camera m_Camera;

		float m_MoveSpeed = 5.0f;
		float m_Boost = 4.2f;
		float m_RotationSpeed = 1.5f;
		float m_RollSpeed = 1.5f;
		float m_WindowSensitivity = 0.002f;

		glm::quat m_Orientation{1,0,0,0};
	};

}
