#pragma once

namespace Wankel {

class SecondOrderDynamics {
public:
    SecondOrderDynamics() = default;

    SecondOrderDynamics(float frequency, float damping, float response, float initialValue);

    float Update(float dt, float target);

    void Reset(float value);

    void SetDynamics(float frequency, float damping, float response);

private:
    float m_K1 = 0.0f;
    float m_K2 = 0.0f;
    float m_K3 = 0.0f;

    float m_PreviousInput {0.0f};

    float m_Output {0.0f};
    float m_OutputVelocity {0.0f};
};

} // namespace Wankel
