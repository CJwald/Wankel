#pragma once

#include "Camera.h"

namespace Wankel {

	class CameraController {
	public:
		CameraController(float aspect);

		void OnUpdate(float deltaTime);

		Camera& GetCamera() { return m_Camera; }

	private:
		Camera m_Camera;

		float m_MoveSpeed = 5.0f;
		float m_RotationSpeed = 0.0015f;
		float m_RollSpeed = 1.5f;

		glm::quat m_Orientation{1,0,0,0};
	};

}
