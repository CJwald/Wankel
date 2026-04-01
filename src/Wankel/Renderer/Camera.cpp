#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Wankel {

Camera::Camera(float fov, float aspect, float nearClip, float farClip)
{
    m_Projection = glm::perspective(glm::radians(fov), aspect, nearClip, farClip);
}

void Camera::SetPosition(const glm::vec3& position)
{
    m_Position = position;
}

const glm::vec3& Camera::GetPosition() const
{
    return m_Position;
}

glm::mat4 Camera::GetViewProjection() const
{
    glm::mat4 view = glm::translate(glm::mat4(1.0f), -m_Position);
    return m_Projection * view;
}

}
