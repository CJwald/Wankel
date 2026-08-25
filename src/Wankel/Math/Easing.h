#pragma once

#include <cstdint>

namespace Wankel {

// Dynamic appended at the end, not inserted between existing values - this enum round-trips through
// JSON as a raw int ordinal (see ComponentSerialization.cpp), so inserting mid-enum would silently
// corrupt every already-saved reference to a later value (e.g. Back).
enum class EaseType : std::uint8_t { Linear, EaseIn, EaseOut, SmoothStep, Sine, Exponential, Back, Dynamic };

// t expected in [0,1] (caller clamps). exponent only used by EaseIn/EaseOut. Back intentionally
// overshoots outside [0,1] partway through - that's the shape, not a bug.
float Ease(EaseType type, float t, float exponent = 2.0f);

} // namespace Wankel
