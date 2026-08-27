#include "Random.h"

#include "Wankel/Math/Math.h"

#include <cmath>

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

glm::vec3 DirectionOnSphere() {
    // Marsaglia (1972) - rejection-free uniform point on the unit sphere.
    float z = Float(-1.0f, 1.0f);
    float phi = Float(0.0f, Math::TAU);
    float r = std::sqrt(glm::max(0.0f, 1.0f - z * z));
    return {r * std::cos(phi), r * std::sin(phi), z};
}

glm::vec3 DirectionInCone(const glm::vec3& axis, float coneAngleDegrees) {
    glm::vec3 n = glm::normalize(axis);

    float cosMax = std::cos(Math::Radians(coneAngleDegrees));
    float cosTheta = Float(cosMax, 1.0f); // uniform over the cap's solid angle
    float sinTheta = std::sqrt(glm::max(0.0f, 1.0f - cosTheta * cosTheta));
    float phi = Float(0.0f, Math::TAU);

    // Orthonormal basis around n - pick a reference axis that isn't near-parallel to n.
    glm::vec3 ref = std::abs(n.y) < 0.999f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 t = glm::normalize(glm::cross(ref, n));
    glm::vec3 b = glm::cross(n, t);

    return sinTheta * std::cos(phi) * t + sinTheta * std::sin(phi) * b + cosTheta * n;
}
} // namespace Wankel::Random
