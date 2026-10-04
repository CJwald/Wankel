#pragma once

#include <glm/glm.hpp>

#include <cstdint>

// Stateless integer hashing for deterministic procedural generation (seed + coordinate -> value).
namespace Wankel::Hash {

// 32-bit avalanche mix (lowbias32) - every input bit affects every output bit.
constexpr uint32_t Mix32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

constexpr uint32_t Combine(uint32_t seed, uint32_t value) {
    return Mix32(seed ^ (value + 0x9E3779B9u + (seed << 6) + (seed >> 2)));
}

inline uint32_t Of(const glm::ivec3& v, uint32_t seed) {
    uint32_t h = Combine(seed, (uint32_t)v.x);
    h = Combine(h, (uint32_t)v.y);
    return Combine(h, (uint32_t)v.z);
}

// [0, 1) from the top 24 bits, exactly representable as a float.
constexpr float ToFloat01(uint32_t h) {
    return (float)(h >> 8) * (1.0f / 16777216.0f);
}

} // namespace Wankel::Hash
