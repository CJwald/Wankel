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
    if (reg.all_of<AABBComponent>(a) && reg.all_of<AABBComponent>(b)) {
        auto& ta = reg.get<TransformComponent>(a);
        auto& tb = reg.get<TransformComponent>(b);

        auto& ca = reg.get<AABBComponent>(a);
        auto& cb = reg.get<AABBComponent>(b);

        AABB A = AABB::FromCenterHalfSize(ta.LocalPosition + ca.Offset, ca.HalfSize);

        AABB B = AABB::FromCenterHalfSize(tb.LocalPosition + cb.Offset, cb.HalfSize);

        out = AABBvsAABB(A, B);
        return out.Colliding;
    }

    // Sphere vs Sphere
    if (reg.all_of<SphereColliderComponent>(a) && reg.all_of<SphereColliderComponent>(b)) {
        auto& ta = reg.get<TransformComponent>(a);
        auto& tb = reg.get<TransformComponent>(b);

        auto& sa = reg.get<SphereColliderComponent>(a);
        auto& sb = reg.get<SphereColliderComponent>(b);

        Sphere A{ ta.LocalPosition, sa.Radius };
        Sphere B{ tb.LocalPosition, sb.Radius };

        out = SpherevsSphere(A, B);
        return out.Colliding;
    }

    // Sphere (A) vs AABB (B)
    if (reg.all_of<SphereColliderComponent>(a) && reg.all_of<AABBComponent>(b)) {
        auto& ta = reg.get<TransformComponent>(a);
        auto& tb = reg.get<TransformComponent>(b);

        auto& sa = reg.get<SphereColliderComponent>(a);
        auto& cb = reg.get<AABBComponent>(b);

        Sphere s{ ta.LocalPosition, sa.Radius };

        AABB box = AABB::FromCenterHalfSize(tb.LocalPosition + cb.Offset, cb.HalfSize);

        out = SpherevsAABB(s, box);
        return out.Colliding;
    }

    // AABB (A) vs Sphere (B)
    if (reg.all_of<AABBComponent>(a) && reg.all_of<SphereColliderComponent>(b)) {
        auto& ta = reg.get<TransformComponent>(a);
        auto& tb = reg.get<TransformComponent>(b);

        auto& ca = reg.get<AABBComponent>(a);
        auto& sb = reg.get<SphereColliderComponent>(b);

        AABB box = AABB::FromCenterHalfSize(ta.LocalPosition + ca.Offset, ca.HalfSize);

        Sphere s{ tb.LocalPosition, sb.Radius };

        // IMPORTANT: keep ordering consistent
        out = SpherevsAABB(s, box);

        // flip to match A's perspective
        out.Normal *= -1.0f;

        return out.Colliding;
    }

    return false;
}

}
