#include "wkpch.h"
#include "PhysicsSystem.h"

#include <Wankel/ECS/Scene.h>
#include <Wankel/ECS/Components.h>

#include "../Collision/BroadPhase/AABB.h"
#include "../Collision/CollisionDispatcher.h"

namespace Wankel {

void PhysicsSystem::Update(Scene& scene, float dt) {

    auto& registry = scene.Registry();

    // INTEGRATE
    {
        auto view = registry.view<TransformComponent, RigidbodyComponent, MovementComponent>();

        for (auto e : view) {
            auto& t = registry.get<TransformComponent>(e);
            auto& rb = registry.get<RigidbodyComponent>(e);
            auto& m = registry.get<MovementComponent>(e);

            if (rb.IsStatic)
            	continue; // if body is static, no integration (go next)

			// ACCELERATION
			float accel = glm::length(m.MoveIntent) > 0.001f ? m.Acceleration : m.Deceleration;

			// VELOCITY
			glm::vec3 targetVel = m.MoveIntent * m.MaxSpeed;
			glm::vec3 deltaVel = targetVel - rb.Velocity;
			float deltaMag = glm::length(deltaVel);

			float maxDV = accel * dt;

			if (deltaMag > maxDV) { 
				deltaVel = glm::normalize(deltaVel) * maxDV;
			}

			rb.Velocity += deltaVel;

			// POSITION
			t.LocalPosition += rb.Velocity * dt;
        }
    }

    // BUILD SPATIAL GRID
    m_Grid.Clear();

    auto buildView = registry.view<TransformComponent, AABBComponent>();

    for (auto e : buildView) {
        auto& t = registry.get<TransformComponent>(e);
        auto& c = registry.get<AABBComponent>(e);

        glm::vec3 center = t.LocalPosition + c.Offset;

        m_Grid.Insert(e, center);
    }

    // COLLISION
    auto view = registry.view<TransformComponent, RigidbodyComponent>();

    for (auto a : view) {
        auto& ta = registry.get<TransformComponent>(a);
        auto& rba = registry.get<RigidbodyComponent>(a);
        auto candidates = m_Grid.Query(ta.LocalPosition);

        for (auto b : candidates) {
            if (a == b)
                continue;

            CollisionManifold manifold;

            if (!ResolveCollision(scene, a, b, manifold))
                continue;

            if (!manifold.Colliding)
                continue;

            auto& tb = registry.get<TransformComponent>(b);
            auto& rbb = registry.get<RigidbodyComponent>(b);

            // POSITION SOLVE
            if (rba.IsStatic && rbb.IsStatic)
                continue;

            if (rba.IsStatic) {
                tb.LocalPosition += manifold.Normal * manifold.Penetration;
            }
            else if (rbb.IsStatic) {
                ta.LocalPosition -= manifold.Normal * manifold.Penetration;
            }
            else {
                ta.LocalPosition -= manifold.Normal * manifold.Penetration * 0.5f;
                tb.LocalPosition += manifold.Normal * manifold.Penetration * 0.5f;
            }

            float va = glm::dot(rba.Velocity, manifold.Normal);

            if (va < 0.0f)
                rba.Velocity -= manifold.Normal * va;
        }
    }
}

}
