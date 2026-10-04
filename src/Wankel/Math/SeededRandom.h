#pragma once

#include "Hash.h"

#include <glm/glm.hpp>

#include <cmath>
#include <cstdint>

namespace Wankel {

// Small value-type RNG (PCG32) for reproducible procedural generation - unlike the global
// Wankel::Random, two instances built from the same seed always produce the same sequence.
class SeededRandom {
public:
    explicit SeededRandom(uint64_t seed) : m_State(0) {
        Next();
        m_State += seed;
        Next();
    }

    uint32_t Next() {
        uint64_t old = m_State;
        m_State = old * 6364136223846793005ULL + 1442695040888963407ULL;
        uint32_t xorShifted = (uint32_t)(((old >> 18u) ^ old) >> 27u);
        uint32_t rot = (uint32_t)(old >> 59u);
        return (xorShifted >> rot) | (xorShifted << ((32u - rot) & 31u));
    }

    float Float() { return Hash::ToFloat01(Next()); } // [0, 1)
    float Float(float min, float max) { return min + (max - min) * Float(); }

    // Inclusive [min, max].
    int Int(int min, int max) {
        if (max <= min)
            return min;
        return min + (int)(Next() % (uint32_t)(max - min + 1));
    }

    glm::vec3 UnitVector() {
        float z = Float(-1.0f, 1.0f);
        float angle = Float(0.0f, 6.28318530718f);
        float r = std::sqrt(glm::max(0.0f, 1.0f - z * z));
        return {r * std::cos(angle), r * std::sin(angle), z};
    }

private:
    uint64_t m_State;
};

namespace Distribution {

// Triangle distribution over [min, max] peaking at `peak`; u is a uniform [0, 1) sample.
inline float SampleTriangle(float min, float peak, float max, float u) {
    if (max <= min)
        return min;
    peak = glm::clamp(peak, min, max);
    float split = (peak - min) / (max - min);
    if (u < split)
        return min + std::sqrt(u * (max - min) * (peak - min));
    return max - std::sqrt((1.0f - u) * (max - min) * (max - peak));
}

// Triangle density at x, normalized so the peak is 1 (handy for rejection sampling).
inline float TriangleShape(float min, float peak, float max, float x) {
    if (x < min || x > max || max <= min)
        return 0.0f;
    peak = glm::clamp(peak, min, max);
    if (x <= peak)
        return peak > min ? (x - min) / (peak - min) : 1.0f;
    return max > peak ? (max - x) / (max - peak) : 1.0f;
}

} // namespace Distribution

} // namespace Wankel
