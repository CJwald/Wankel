#include "wkpch.h"
#include "Wankel/Renderer/Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Wankel {

Camera::Camera(float fov, float aspect, float nearClip, float farClip)
    : m_FOV(fov), m_Aspect(aspect), m_Near(nearClip), m_Far(farClip) {}

void Camera::SetPosition(const glm::vec3& position) {
    m_Position = position;
}

void Camera::SetFOV(const float& fov) {
    if (fov <= 0.0f || fov >= 180.0f) {
        WK_CORE_WARNING("Camera::SetFOV - {0} out of range (0, 180), clamping", fov);
        m_FOV = glm::clamp(fov, 0.01f, 179.99f);
        return;
    }
    m_FOV = fov;
}

void Camera::SetAspect(const float& aspect) {
    if (aspect <= 0.0f) {
        WK_CORE_WARNING("Camera::SetAspect - {0} must be > 0, ignoring", aspect);
        return;
    }
    m_Aspect = aspect;
}

void Camera::SetNearClip(const float& nearClip) {
    if (nearClip <= 0.0f || nearClip >= m_Far) {
        WK_CORE_WARNING("Camera::SetNearClip - {0} must be > 0 and < far clip ({1}), ignoring", nearClip, m_Far);
        return;
    }
    m_Near = nearClip;
}

void Camera::SetFarClip(const float& farClip) {
    if (farClip <= m_Near) {
        WK_CORE_WARNING("Camera::SetFarClip - {0} must be > near clip ({1}), ignoring", farClip, m_Near);
        return;
    }
    m_Far = farClip;
}

const glm::vec3& Camera::GetPosition() const {
    return m_Position;
}

void Camera::SetOrientation(const glm::quat& orientation) {
    m_Orientation = glm::normalize(orientation);
}

const glm::quat& Camera::GetOrientation() const {
    return m_Orientation;
}

glm::vec3 Camera::GetForward() const {
    return m_Orientation * glm::vec3(0, 0, -1);
}

glm::vec3 Camera::GetRight() const {
    return m_Orientation * glm::vec3(1, 0, 0);
}

glm::vec3 Camera::GetUp() const {
    return m_Orientation * glm::vec3(0, 1, 0);
}

float Camera::GetFOV() const {
    return m_FOV;
}

float Camera::GetNearClip() const {
    return m_Near;
}

float Camera::GetFarClip() const {
    return m_Far;
}

float Camera::GetAspect() const {
    return m_Aspect;
}

glm::mat4 Camera::GetViewMatrix() const {
    glm::mat4 rotation = glm::toMat4(glm::conjugate(m_Orientation));
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), -m_Position);
    return rotation * translation;
}

glm::mat4 Camera::GetProjectionMatrix() const {
    return glm::perspective(glm::radians(m_FOV), m_Aspect, m_Near, m_Far);
}

} // namespace Wankel
