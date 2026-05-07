#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Wankel {

	class Camera {
	public:
		Camera(float fov, float aspect, float nearClip, float farClip);

		// Position
		void SetPosition(const glm::vec3& position);
		void SetFOV(const float& fov);
		void SetAspect(const float& aspect);
		void SetNearClip(const float& nearClip);
		void SetFarClip(const float& farClip);
		const glm::vec3& GetPosition() const;

		// Rotation
		void SetOrientation(const glm::quat& orientation);
		const glm::quat& GetOrientation() const;

		// Movement directions
		glm::vec3 GetForward() const;
		glm::vec3 GetRight() const;
		glm::vec3 GetUp() const;
		
		float GetFOV() const;
		float GetNearClip() const;
		float GetFarClip() const;
		float GetAspect() const;

		// Matrices
		glm::mat4 GetViewMatrix() const;
		glm::mat4 GetProjectionMatrix() const;

	private:
		glm::vec3 m_Position{0.0f, 0.0f, 3.0f};
		glm::quat m_Orientation{1, 0, 0, 0}; // identity

		float m_FOV;
		float m_Aspect;
		float m_Near, m_Far;
	};

}
