#include "wkpch.h"

#include "CollisionDispatcher.h"

#include <Wankel/ECS/Scene.h>
#include <Wankel/ECS/Components.h>

#include "BroadPhase/AABB.h"
#include "NarrowPhase/AABBCollision.h"
#include "NarrowPhase/SphereCollision.h"
#include "NarrowPhase/SphereAABBCollision.h"

namespace Wankel {

bool ResolveCollision(Scene& scene, entt::entity a, entt::entity b, CollisionManifold& out) {
    auto& reg = scene.Registry();

    // AABB vs AABB
    if (reg.all_of<AABBCollider>(a) && reg.all_of<AABBCollider>(b)) {
        auto& ta = reg.get<Transform>(a);
        auto& tb = reg.get<Transform>(b);

        auto& ca = reg.get<AABBCollider>(a);
        auto& cb = reg.get<AABBCollider>(b);

        AABB A = AABB::FromCenterHalfSize(ta.LocalPosition + ca.Offset, ca.HalfSize);

        AABB B = AABB::FromCenterHalfSize(tb.LocalPosition + cb.Offset, cb.HalfSize);

        out = AABBvsAABB(A, B);
        return out.Colliding;
    }

    // Sphere vs Sphere
    if (reg.all_of<SphereCollider>(a) && reg.all_of<SphereCollider>(b)) {
        auto& ta = reg.get<Transform>(a);
        auto& tb = reg.get<Transform>(b);

        auto& sa = reg.get<SphereCollider>(a);
        auto& sb = reg.get<SphereCollider>(b);

        Sphere A{ ta.LocalPosition, sa.Radius };
        Sphere B{ tb.LocalPosition, sb.Radius };

        out = SpherevsSphere(A, B);
        return out.Colliding;
    }

    // Sphere (A) vs AABB (B)
    if (reg.all_of<SphereCollider>(a) && reg.all_of<AABBCollider>(b)) {
        auto& ta = reg.get<Transform>(a);
        auto& tb = reg.get<Transform>(b);

        auto& sa = reg.get<SphereCollider>(a);
        auto& cb = reg.get<AABBCollider>(b);

        Sphere s{ ta.LocalPosition, sa.Radius };

        AABB box = AABB::FromCenterHalfSize(tb.LocalPosition + cb.Offset, cb.HalfSize);

        // SpherevsAABB returns a normal pointing box->sphere (B->A here);
        // flip so the manifold normal points A->B, matching AABBvsAABB/SpherevsSphere.
        out = SpherevsAABB(s, box);
        out.Normal *= -1.0f;

        return out.Colliding;
    }

    // AABB (A) vs Sphere (B)
    if (reg.all_of<AABBCollider>(a) && reg.all_of<SphereCollider>(b)) {
        auto& ta = reg.get<Transform>(a);
        auto& tb = reg.get<Transform>(b);

        auto& ca = reg.get<AABBCollider>(a);
        auto& sb = reg.get<SphereCollider>(b);

        AABB box = AABB::FromCenterHalfSize(ta.LocalPosition + ca.Offset, ca.HalfSize);

        Sphere s{ tb.LocalPosition, sb.Radius };

        // SpherevsAABB returns a normal pointing box->sphere, which is
        // already A->B here (A=box, B=sphere) -- no flip needed.
        out = SpherevsAABB(s, box);

        return out.Colliding;
    }

    return false;
}

}
