#include "wkpch.h"
#include "Easing.h"

#include <cmath>

#include "Math.h"

namespace Wankel {

float Ease(EaseType type, float t, float exponent) {
    switch (type) {
        case EaseType::Linear:
            return t;
        case EaseType::EaseIn:
            return std::pow(t, exponent);
        case EaseType::EaseOut:
            return 1.0f - std::pow(1.0f - t, exponent);
        case EaseType::SmoothStep:
            return Math::SmoothStep(t);
        case EaseType::Sine:
            return -(std::cos(Math::PI * t) - 1.0f) / 2.0f;
        case EaseType::Exponential:
            if (t <= 0.0f)
                return 0.0f;
            if (t >= 1.0f)
                return 1.0f;
            return t < 0.5f ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f
                            : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
        case EaseType::Back: {
            constexpr float c1 = 1.70158f;
            constexpr float c2 = c1 * 1.525f;
            return t < 0.5f ? (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
                            : (std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
        }
    }
    return t;
}

} // namespace Wankel
