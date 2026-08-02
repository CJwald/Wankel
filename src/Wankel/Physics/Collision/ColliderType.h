#pragma once

#include <glm/glm.hpp>

namespace Wankel {

class TriangleMesh;

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

    glm::vec3 Center {0.0f};

    glm::vec3 HalfSize {0.0f}; // AABB
    float Radius = 0.0f;       // Sphere / Capsule
    float HalfHeight = 0.0f;   // Capsule

    // Non-owning - MeshCollider owns the Ref<TriangleMesh>; this struct is
    // built fresh per broad-phase candidate and scoped to one
    // ResolveCollision call while the registry/component is guaranteed alive.
    const TriangleMesh* Mesh = nullptr;

    // Copied from whichever concrete collider component this shape was extracted from (see
    // ExtractShape in CollisionDispatcher.cpp) - combined with the other side's own value into
    // CollisionManifold::Friction once both shapes are known.
    float Friction = 0.0f;
};

} // namespace Wankel
