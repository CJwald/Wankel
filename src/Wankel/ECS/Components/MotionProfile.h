#pragma once

#include "Wankel/Math/SecondOrderDynamics.h"

namespace Wankel {

enum class MotionAxis : uint8_t {
    X = 0,
    Y,
    Z,
    Pitch,
    Yaw,
    Roll,

    Count
};

// Shared with the debug/inspector panels and the component serializer, so all three agree on one
// name per axis.
inline const char* MotionAxisName(MotionAxis axis) {
    switch (axis) {
        case MotionAxis::X:
            return "X";
        case MotionAxis::Y:
            return "Y";
        case MotionAxis::Z:
            return "Z";
        case MotionAxis::Pitch:
            return "Pitch";
        case MotionAxis::Yaw:
            return "Yaw";
        case MotionAxis::Roll:
            return "Roll";
        case MotionAxis::Count:
            break;
    }
    return "Unknown";
}

struct MotionLink {
    bool Enabled = false;

    float Magnitude = 0.0f;

    float Frequency = 2.0f;
    float Damping = 0.8f;
    float Response = 2.0f;

    float ClampMin = -9999.0f;
    float ClampMax = 9999.0f;

    SecondOrderDynamics Spring = SecondOrderDynamics(2.0f, 0.8f, 2.0f, 0.0f);

    float Output = 0.0f;

    // Copies only the authored tuning, not Spring/Output (live runtime state) - lets a pose swap
    // sway "feel" without resetting the spring's current physical motion, matching how
    // ProceduralAnimationSystem already re-tunes Spring from these fields every frame regardless.
    void CopyTuning(const MotionLink& src) {
        Enabled = src.Enabled;
        Magnitude = src.Magnitude;
        Frequency = src.Frequency;
        Damping = src.Damping;
        Response = src.Response;
        ClampMin = src.ClampMin;
        ClampMax = src.ClampMax;
    }
};

} // namespace Wankel
