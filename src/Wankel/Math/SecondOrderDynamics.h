#pragma once

#include <glm/glm.hpp>

namespace Wankel {

class SecondOrderDynamics {
public:

    SecondOrderDynamics() = default;

    SecondOrderDynamics(
        float frequency,
        float damping,
        float response,
        glm::vec3 initialValue
    );

    glm::vec3 Update(
        float dt,
        glm::vec3 target
    );

    void Reset(glm::vec3 value);

	void SetDynamics(float frequency, float damping, float response);

private:

    float m_K1 = 0.0f;
    float m_K2 = 0.0f;
    float m_K3 = 0.0f;

    glm::vec3 m_PreviousInput{0.0f};

    glm::vec3 m_Output{0.0f};
    glm::vec3 m_OutputVelocity{0.0f};
};

}
