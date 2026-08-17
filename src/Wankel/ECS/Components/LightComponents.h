#pragma once
#include <glm/glm.hpp>

namespace Wankel {

// Emits light from the owning entity's world position in all directions,
// falling off with distance. Pair with Transform (position comes from
// Transform::FinalTransform each frame) - same pairing as MeshRenderer.
struct PointLight {
    glm::vec3 Color = {1.0f, 1.0f, 1.0f};
    float Intensity = 1.0f;
    float Radius = 10.0f; // distance at which attenuation reaches ~0
};

} // namespace Wankel
