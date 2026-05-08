#pragma once

#include <random>

namespace Wankel::Random {
    void Init(uint32_t seed);

    float Float();

    float Float(float min, float max);

    int Int(int min, int max);
}
