#include "Random.h"

namespace Wankel::Random {

    static std::mt19937 s_RNG;

    void Init(uint32_t seed) {
        s_RNG.seed(seed);
    }

    float Float() {
        return Float(0.0f, 1.0f);
    }

    float Float(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(s_RNG);
    }

    int Int(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(s_RNG);
    }
}
