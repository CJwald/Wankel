#pragma once

namespace Wankel {

class SecondOrderDynamics {
public:
    SecondOrderDynamics() = default;

    SecondOrderDynamics(float frequency, float damping, float response, float initialValue);

    float Update(float dt, float target);

    // Bumps the spring's internal velocity directly, independent of target - a one-shot kick that
    // Update() then naturally integrates/settles back down per the spring's own f/z/r tuning.
    void AddImpulse(float velocityDelta);

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
