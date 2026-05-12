#include "wkpch.h"
#include "PhysicsSystem.h"

#include <Wankel/ECS/Scene.h>
#include <Wankel/ECS/Components.h>

#include "../Collision/BroadPhase/AABB.h"
#include "../Collision/NarrowPhase/AABBCollision.h"

namespace Wankel {

void PhysicsSystem::Update(Scene& scene, float dt)
{
    auto& registry = scene.Registry();

    // =========================
    // INTEGRATE
    // =========================
    {
        auto view = registry.view<TransformComponent, RigidbodyComponent>();

        for (auto e : view)
        {
            auto& t = view.get<TransformComponent>(e);
            auto& rb = view.get<RigidbodyComponent>(e);

            if (!rb.IsStatic) {
				rb.Velocity = rb.ForcedVelocity;
                t.LocalPosition += rb.Velocity * dt;
			}
        }
    }

    // =========================
    // BUILD SPATIAL GRID
    // =========================
    m_Grid.Clear();

    auto buildView = registry.view<TransformComponent, AABBComponent>();

    for (auto e : buildView)
    {
        auto& t = buildView.get<TransformComponent>(e);
        m_Grid.Insert(e, t.LocalPosition);
    }

    // =========================
    // COLLISION (OPTIMIZED)
    // =========================
    auto view = registry.view<TransformComponent, AABBComponent, RigidbodyComponent>();

    for (auto a : view)
    {
        auto& ta = registry.get<TransformComponent>(a);
        auto& ca = registry.get<AABBComponent>(a);
        auto& rba = registry.get<RigidbodyComponent>(a);

        AABB aabbA = AABB::FromCenterHalfSize(ta.LocalPosition, ca.HalfSize);

        auto candidates = m_Grid.Query(ta.LocalPosition);

        for (auto b : candidates)
        {
            if (a == b) continue;

            if (!registry.all_of<TransformComponent, AABBComponent, RigidbodyComponent>(b))
                continue;

            auto& tb = registry.get<TransformComponent>(b);
            auto& cb = registry.get<AABBComponent>(b);
            auto& rbb = registry.get<RigidbodyComponent>(b);

            AABB aabbB = AABB::FromCenterHalfSize(tb.LocalPosition, cb.HalfSize);

            auto manifold = AABBvsAABB(aabbA, aabbB);

            if (!manifold.Colliding)
                continue;

            // =========================
            // POSITION RESOLUTION
            // =========================
            if (rba.IsStatic && rbb.IsStatic)
                continue;

            if (rba.IsStatic)
            {
                tb.LocalPosition += manifold.Normal * manifold.Penetration;
            }
            else if (rbb.IsStatic)
            {
                ta.LocalPosition -= manifold.Normal * manifold.Penetration;
            }
            else
            {
                ta.LocalPosition -= manifold.Normal * manifold.Penetration * 0.5f;
                tb.LocalPosition += manifold.Normal * manifold.Penetration * 0.5f;
            }

            // =========================
            // VELOCITY FIX
            // =========================
            float va = glm::dot(rba.Velocity, manifold.Normal);
            if (va < 0.0f)
                rba.Velocity -= manifold.Normal * va;
        }
    }
}
}
