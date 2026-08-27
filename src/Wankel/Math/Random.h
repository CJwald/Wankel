#pragma once

#include <glm/glm.hpp>

#include <random>

namespace Wankel::Random {
void Init(uint32_t seed);

float Float();

float Float(float min, float max);

int Int(int min, int max);

// Uniformly distributed unit vector over the whole sphere.
glm::vec3 DirectionOnSphere();

// Uniformly distributed (over solid angle) unit vector within coneAngleDegrees of `axis`. A
// coneAngleDegrees of 0 returns `axis` normalized; 180 covers the full sphere.
glm::vec3 DirectionInCone(const glm::vec3& axis, float coneAngleDegrees);
} // namespace Wankel::Random
