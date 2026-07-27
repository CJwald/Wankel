#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Wankel/Core/Base.h"
#include "Wankel/Physics/Collision/TriangleMesh.h"


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


// Static triangle-mesh collider (terrain). Translation-only (Offset, like
// every other collider here) - no rotation/scale, appropriate for
// axis-aligned voxel chunks. Owned directly here rather than via
// AssetManager: AssetManager's cache is permanent-until-global-Clear(),
// which is the wrong lifetime for voxel chunks that load/unload/regenerate
// as the player moves - whatever terrain-chunk system builds this Mesh
// owns it for exactly as long as this component exists.
// Must not be reassigned mid-PhysicsSystem::Update (single-threaded
// assumption - fine today, flag this if chunk streaming ever runs
// collision-triggered regeneration mid-tick).
struct MeshCollider {
    Ref<TriangleMesh> Mesh;
    glm::vec3 Offset {0.0f};
};

} // namespace Wankel
