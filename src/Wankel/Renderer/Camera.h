#pragma once
#include <glm/glm.hpp>

namespace Wankel {

class Camera {
public:
    Camera(float fov, float aspect, float nearClip, float farClip);

    void SetPosition(const glm::vec3& position);
    const glm::vec3& GetPosition() const;

    glm::mat4 GetViewProjection() const;

private:
    glm::mat4 m_Projection;
    glm::vec3 m_Position = {0.0f, 0.0f, 3.0f};
};

}
