#pragma once

#include <glm/glm.hpp>

namespace Wankel {

enum class ColliderType {
    None = 0,

    AABB,
    Sphere,
    Capsule,
    Mesh
};

// Type-erased, world-space view of a collider's shape data. Narrow-phase
// dispatch is keyed on ColliderShape::Type instead of branching on concrete
// collider component types, so adding a new collider type doesn't require
// touching every existing pair.
struct ColliderShape {
    ColliderType Type = ColliderType::None;

    glm::vec3 Center{0.0f};

    glm::vec3 HalfSize{0.0f};  // AABB
    float Radius = 0.0f;       // Sphere / Capsule
    float HalfHeight = 0.0f;   // Capsule
};

}
