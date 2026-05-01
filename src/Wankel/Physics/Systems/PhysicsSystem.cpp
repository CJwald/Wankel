#include "wkpch.h"
#include "PhysicsSystem.h"

#include "../Collision/BroadPhase/AABB.h"
#include "../Collision/NarrowPhase/AABBCollision.h"

#include <Wankel/ECS/Components.h>
#include <Wankel/ECS/Scene.h>

namespace Wankel {

void PhysicsSystem::Update(Scene& scene, float dt)
{
    auto& registry = scene.Registry();

    auto view = registry.view<TransformComponent, AABBComponent>();

    // Iterate pairs ONCE (no double resolution)
    for (auto itA = view.begin(); itA != view.end(); ++itA)
    {
        auto entityA = *itA;

        auto& ta = registry.get<TransformComponent>(entityA);
        auto& ca = registry.get<AABBComponent>(entityA);

        AABB aabbA = AABB::FromCenterHalfSize(ta.Position, ca.HalfSize);

        auto itB = itA;
        ++itB;

        for (; itB != view.end(); ++itB)
        {
            auto entityB = *itB;

            auto& tb = registry.get<TransformComponent>(entityB);
            auto& cb = registry.get<AABBComponent>(entityB);

            AABB aabbB = AABB::FromCenterHalfSize(tb.Position, cb.HalfSize);

            CollisionManifold manifold = AABBvsAABB(aabbA, aabbB);

            if (!manifold.Colliding) {
                continue;
			} else {
				WK_CORE_INFO("COLLIDING");
			}

            // =========================
            // STATIC / DYNAMIC HANDLING
            // =========================
            bool aStatic = false;
            bool bStatic = false;

            if (registry.all_of<RigidbodyComponent>(entityA))
                aStatic = registry.get<RigidbodyComponent>(entityA).IsStatic;

            if (registry.all_of<RigidbodyComponent>(entityB))
                bStatic = registry.get<RigidbodyComponent>(entityB).IsStatic;

            // both static → do nothing
            if (aStatic && bStatic)
                continue;

            // =========================
            // RESOLUTION
            // =========================
            if (aStatic)
            {
                tb.Position += manifold.Normal * manifold.Penetration;
            }
            else if (bStatic)
            {
                ta.Position -= manifold.Normal * manifold.Penetration;
            }
            else
            {
                // split movement
                ta.Position -= manifold.Normal * manifold.Penetration * 0.5f;
                tb.Position += manifold.Normal * manifold.Penetration * 0.5f;
            }
        }
    }
}

}
