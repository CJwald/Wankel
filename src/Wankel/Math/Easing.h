#pragma once

#include <cstdint>

namespace Wankel {

enum class EaseType : std::uint8_t { Linear, EaseIn, EaseOut, SmoothStep, Sine, Exponential, Back };

// t expected in [0,1] (caller clamps). exponent only used by EaseIn/EaseOut. Back intentionally
// overshoots outside [0,1] partway through - that's the shape, not a bug.
float Ease(EaseType type, float t, float exponent = 2.0f);

} // namespace Wankel
