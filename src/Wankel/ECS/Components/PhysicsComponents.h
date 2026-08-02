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

    // Per-entity multiplier on PhysicsSystem::Gravity (see PhysicsSystem::Update) - 1.0 (full gravity)
    // by default, matching what any generic Rigidbody (props, ragdolls) should expect. A
    // PlayerController-driven entity gets this overwritten every frame based on look mode instead
    // (PlayerControllerSystem) - see PlayerController::FlightGravityScale.
    float GravityScale = 1.0f;
};


// Friction default (0.6) is a moderate, "concrete/wood"-ish coefficient - not zero, so any collider
// nobody's explicitly tuned yet still gets a reasonable stop-sliding-on-a-shallow-slope response
// rather than silently reverting to the old frictionless behavior. See CollisionManifold::Friction
// for how two colliders' values combine, and PhysicsSystem::Update for how it's actually applied.
struct AABBCollider {
    glm::vec3 HalfSize = {0.5f, 0.5f, 0.5f};
    glm::vec3 Offset {0.0f};
    float Friction = 0.6f;
};


struct SphereCollider {
    float Radius = 0.5f;
    glm::vec3 Offset {0.0f};
    float Friction = 0.6f;
};


// Upright capsule (segment along world Y, swept by Radius) - see
// Physics/Collision/NarrowPhase/Capsule.h for the narrow-phase shape.
// HalfHeight is half the length of the *segment*, not the overall capsule
// height (overall height = 2*HalfHeight + 2*Radius).
struct CapsuleCollider {
    float Radius = 0.5f;
    float HalfHeight = 0.5f;
    glm::vec3 Offset {0.0f};
    float Friction = 0.6f;
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
    float Friction = 0.6f;
};

} // namespace Wankel
