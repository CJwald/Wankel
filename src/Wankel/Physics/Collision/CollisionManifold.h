#pragma once
#include <glm/glm.hpp>

namespace Wankel {

struct CollisionManifold {
    bool Colliding = false;
    glm::vec3 Normal = {0, 0, 0};
    float Penetration = 0.0f;

    // Combined Coulomb friction coefficient for this contact - geometric mean of both colliders' own
    // ColliderShape::Friction (see CollisionDispatcher::ResolveCollision), the same default
    // combination rule most physics engines use. 0 for narrow-phase functions that don't set it
    // (there are none left to worry about - ResolveCollision sets this after every dispatch).
    float Friction = 0.0f;
};

} // namespace Wankel
