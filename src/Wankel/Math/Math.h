#pragma once

#include <algorithm>
#include <cmath>

namespace Wankel::Math {
    constexpr float PI = 3.14159265359f;
    constexpr float TAU = 6.28318530718f;

    template<typename T>
    constexpr T Clamp(T value, T min, T max) {
        return std::clamp(value, min, max);
    }

    template<typename T>
    constexpr T Lerp(T a, T b, float t) {
        return a + (b - a) * t;
    }

    inline float Degrees(float radians) {
        return radians * (180.0f / PI);
    }

    inline float Radians(float degrees) {
        return degrees * (PI / 180.0f);
    }

    inline float SmoothStep(float t) {
        return t * t * (3.0f - 2.0f * t);
    }

    inline float SmootherStep(float t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }
}
