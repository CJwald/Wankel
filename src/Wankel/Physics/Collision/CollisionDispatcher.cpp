#include "wkpch.h"

#include "CollisionDispatcher.h"

#include <array>

#include <Wankel/ECS/Scene.h>
#include <Wankel/ECS/Components.h>

#include "ColliderType.h"
#include "BroadPhase/AABB.h"
#include "NarrowPhase/AABBCollision.h"
#include "NarrowPhase/SphereCollision.h"
#include "NarrowPhase/SphereAABBCollision.h"
#include "NarrowPhase/Capsule.h"
#include "NarrowPhase/CapsuleCollision.h"
#include "NarrowPhase/SphereCapsuleCollision.h"
#include "NarrowPhase/CapsuleAABBCollision.h"
#include "NarrowPhase/SphereMeshCollision.h"
#include "NarrowPhase/CapsuleMeshCollision.h"
#include "NarrowPhase/AABBMeshCollision.h"

namespace Wankel {

namespace {

constexpr size_t kColliderTypeCount = static_cast<size_t>(ColliderType::Mesh) + 1;

size_t Idx(ColliderType type) {
    return static_cast<size_t>(type);
}

bool ExtractShape(entt::registry& reg, entt::entity e, ColliderShape& out) {
    auto& transform = reg.get<Transform>(e);

    if (auto* c = reg.try_get<AABBCollider>(e)) {
        out.Type = ColliderType::AABB;
        out.Center = transform.LocalPosition + c->Offset;
        out.HalfSize = c->HalfSize;
        out.Friction = c->Friction;
        return true;
    }

    if (auto* c = reg.try_get<SphereCollider>(e)) {
        out.Type = ColliderType::Sphere;
        out.Center = transform.LocalPosition + c->Offset;
        out.Radius = c->Radius;
        out.Friction = c->Friction;
        return true;
    }

    if (auto* c = reg.try_get<CapsuleCollider>(e)) {
        out.Type = ColliderType::Capsule;
        out.Center = transform.LocalPosition + c->Offset;
        out.Radius = c->Radius;
        out.HalfHeight = c->HalfHeight;
        out.Friction = c->Friction;
        return true;
    }

    if (auto* c = reg.try_get<MeshCollider>(e)) {
        if (!c->Mesh)
            return false; // collider attached but not yet populated - treat as absent, not a crash

        out.Type = ColliderType::Mesh;
        out.Center = transform.LocalPosition + c->Offset; // doubles as the mesh's world origin
        out.Mesh = c->Mesh.get();
        out.Friction = c->Friction;
        return true;
    }

    return false;
}

AABB ToAABB(const ColliderShape& s) {
    return AABB::FromCenterHalfSize(s.Center, s.HalfSize);
}

Sphere ToSphere(const ColliderShape& s) {
    return Sphere {s.Center, s.Radius};
}

Capsule ToCapsule(const ColliderShape& s) {
    return Capsule {s.Center, s.Radius, s.HalfHeight};
}

using NarrowPhaseFn = CollisionManifold (*)(const ColliderShape&, const ColliderShape&);
using NarrowPhaseTable = std::array<std::array<NarrowPhaseFn, kColliderTypeCount>, kColliderTypeCount>;

// table[lo][hi], lo <= hi by ColliderType value, holds the narrow-phase
// function for that unordered type pair. Its manifold normal points from
// the lo-type shape to the hi-type shape; ResolveCollision flips the
// normal when the caller's (a, b) order is the reverse of (lo, hi).
NarrowPhaseTable BuildNarrowPhaseTable() {
    NarrowPhaseTable table {};

    table[Idx(ColliderType::AABB)][Idx(ColliderType::AABB)] = [](const ColliderShape& a, const ColliderShape& b) {
        return AABBvsAABB(ToAABB(a), ToAABB(b));
    };

    table[Idx(ColliderType::Sphere)][Idx(ColliderType::Sphere)] = [](const ColliderShape& a, const ColliderShape& b) {
        return SpherevsSphere(ToSphere(a), ToSphere(b));
    };

    table[Idx(ColliderType::AABB)][Idx(ColliderType::Sphere)] = [](const ColliderShape& box,
                                                                   const ColliderShape& sphere) {
        return SpherevsAABB(ToSphere(sphere), ToAABB(box));
    };

    table[Idx(ColliderType::Capsule)][Idx(ColliderType::Capsule)] = [](const ColliderShape& a, const ColliderShape& b) {
        return CapsulevsCapsule(ToCapsule(a), ToCapsule(b));
    };

    table[Idx(ColliderType::Sphere)][Idx(ColliderType::Capsule)] = [](const ColliderShape& sphere,
                                                                      const ColliderShape& capsule) {
        return SpherevsCapsule(ToSphere(sphere), ToCapsule(capsule));
    };

    table[Idx(ColliderType::AABB)][Idx(ColliderType::Capsule)] = [](const ColliderShape& box,
                                                                    const ColliderShape& capsule) {
        return CapsulevsAABB(ToCapsule(capsule), ToAABB(box));
    };

    table[Idx(ColliderType::Sphere)][Idx(ColliderType::Mesh)] = [](const ColliderShape& sphere,
                                                                   const ColliderShape& mesh) {
        return SpherevsMesh(ToSphere(sphere), mesh.Center, *mesh.Mesh);
    };

    table[Idx(ColliderType::AABB)][Idx(ColliderType::Mesh)] = [](const ColliderShape& box, const ColliderShape& mesh) {
        return AABBvsMesh(ToAABB(box), mesh.Center, *mesh.Mesh);
    };

    table[Idx(ColliderType::Capsule)][Idx(ColliderType::Mesh)] = [](const ColliderShape& capsule,
                                                                    const ColliderShape& mesh) {
        return CapsulevsMesh(ToCapsule(capsule), mesh.Center, *mesh.Mesh);
    };

    return table;
}

const NarrowPhaseTable& GetNarrowPhaseTable() {
    static const NarrowPhaseTable table = BuildNarrowPhaseTable();
    return table;
}

} // namespace

bool ResolveCollision(Scene& scene, entt::entity a, entt::entity b, CollisionManifold& out) {
    auto& reg = scene.Registry();

    ColliderShape shapeA, shapeB;

    if (!ExtractShape(reg, a, shapeA))
        return false;
    if (!ExtractShape(reg, b, shapeB))
        return false;

    size_t ia = Idx(shapeA.Type);
    size_t ib = Idx(shapeB.Type);
    bool swapped = ia > ib;

    NarrowPhaseFn fn = swapped ? GetNarrowPhaseTable()[ib][ia] : GetNarrowPhaseTable()[ia][ib];

    if (!fn) {
        WK_CORE_WARNING("ResolveCollision: no narrow-phase dispatch for collider type pair ({0}, {1})", ia, ib);
        return false;
    }

    out = swapped ? fn(shapeB, shapeA) : fn(shapeA, shapeB);
    if (swapped)
        out.Normal *= -1.0f;

    // Geometric mean - same default combination rule most physics engines use for two contacting
    // materials' friction coefficients (a low-friction side still meaningfully reduces the combined
    // result, unlike a plain average).
    out.Friction = glm::sqrt(shapeA.Friction * shapeB.Friction);

    return out.Colliding;
}

} // namespace Wankel
