#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>


namespace Wankel {

struct Rigidbody {
    glm::vec3 Velocity {0.0f};
    glm::vec3 Force {0.0f}; // Not used, I think I want it eventually

    float Mass = 1.0f;
    bool IsStatic = false;
};


struct AABBCollider {
    glm::vec3 HalfSize = {0.5f, 0.5f, 0.5f};
    glm::vec3 Offset {0.0f};
};


struct SphereCollider {
    float Radius = 0.5f;
    glm::vec3 Offset {0.0f};
};


// Upright capsule (segment along world Y, swept by Radius) - see
// Physics/Collision/NarrowPhase/Capsule.h for the narrow-phase shape.
// HalfHeight is half the length of the *segment*, not the overall capsule
// height (overall height = 2*HalfHeight + 2*Radius).
struct CapsuleCollider {
    float Radius = 0.5f;
    float HalfHeight = 0.5f;
    glm::vec3 Offset {0.0f};
};

} // namespace Wankel
