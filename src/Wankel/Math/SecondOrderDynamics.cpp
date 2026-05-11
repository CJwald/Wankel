#include "wkpch.h"
#include "SecondOrderDynamics.h"

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

namespace Wankel {

static constexpr float PI = 3.14159265359f;

SecondOrderDynamics::SecondOrderDynamics(
    float frequency,
    float damping,
    float response,
    glm::vec3 initialValue)
{
    m_K1 =
        damping / (PI * frequency);

    m_K2 =
        1.0f /
        ((2.0f * PI * frequency) *
         (2.0f * PI * frequency));

    m_K3 =
        response * damping /
        (2.0f * PI * frequency);

    m_PreviousInput = initialValue;

    m_Output = initialValue;

    m_OutputVelocity = glm::vec3(0.0f);
}

glm::vec3 SecondOrderDynamics::Update(
    float dt,
    glm::vec3 target)
{
    glm::vec3 inputVelocity =
        (target - m_PreviousInput) / dt;

    m_PreviousInput = target;

    float stableK2 =
        std::max(
            m_K2,
            1.1f *
            (dt * dt / 4.0f +
             dt * m_K1 / 2.0f)
        );

    // Integrate position
    m_Output +=
        m_OutputVelocity * dt;

    // Acceleration
    glm::vec3 acceleration =
        (target
        + m_K3 * inputVelocity
        - m_Output
        - m_K1 * m_OutputVelocity)
        / stableK2;

    // Integrate velocity
    m_OutputVelocity +=
        acceleration * dt;

    return m_Output;
}

void SecondOrderDynamics::Reset(glm::vec3 value)
{
    m_PreviousInput = value;
    m_Output = value;
    m_OutputVelocity = glm::vec3(0.0f);
}

void SecondOrderDynamics::SetDynamics(
    float frequency,
    float damping,
    float response)
{
    m_K1 =
        damping / (PI * frequency);

    m_K2 =
        1.0f /
        ((2.0f * PI * frequency) *
         (2.0f * PI * frequency));

    m_K3 =
        response * damping /
        (2.0f * PI * frequency);
}

}
