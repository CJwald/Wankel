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

    // Movement-driven velocity target-seeking (character controllers etc.)
    {
        auto view = registry.view<Rigidbody, Movement>();

        for (auto e : view) {
            auto& rb = registry.get<Rigidbody>(e);
            auto& m = registry.get<Movement>(e);

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
        }
    }

    // Position integration applies to every dynamic rigidbody, whether or
    // not it has a Movement component (e.g. thrown props, ragdolls, anything
    // whose velocity comes purely from collision response).
    {
        auto view = registry.view<Transform, Rigidbody>();

        for (auto e : view) {
            auto& t = registry.get<Transform>(e);
            auto& rb = registry.get<Rigidbody>(e);

            if (rb.IsStatic)
                continue;

            t.LocalPosition += rb.Velocity * dt;
        }
    }

    // BUILD SPATIAL GRID
    m_Grid.Clear();

    auto buildView = registry.view<Transform, AABBCollider>();

    for (auto e : buildView) {
        auto& t = registry.get<Transform>(e);
        auto& c = registry.get<AABBCollider>(e);

        glm::vec3 center = t.LocalPosition + c.Offset;

        m_Grid.Insert(e, center);
    }

    // COLLISION
    auto view = registry.view<Transform, Rigidbody>();

    // The broad-phase grid only contains AABBCollider entities, so a pair
    // can be discovered from either side (both AABB) or just one side
    // (e.g. sphere-vs-AABB, only found while iterating the sphere). Track
    // pairs already resolved this frame by canonical (min, max) entity key
    // so a symmetric discovery doesn't resolve the same pair twice, without
    // assuming every pair is discovered from both directions.
    std::unordered_set<uint64_t> resolvedPairs;

    for (auto a : view) {
        auto& ta = registry.get<Transform>(a);
        auto& rba = registry.get<Rigidbody>(a);
        auto candidates = m_Grid.Query(ta.LocalPosition);

        for (auto b : candidates) {
            if (a == b)
                continue;

            uint32_t idA = entt::to_integral(a);
            uint32_t idB = entt::to_integral(b);
            uint64_t pairKey = ((uint64_t)std::min(idA, idB) << 32) | std::max(idA, idB);

            if (!resolvedPairs.insert(pairKey).second)
                continue;

            CollisionManifold manifold;

            if (!ResolveCollision(scene, a, b, manifold))
                continue;

            if (!manifold.Colliding)
                continue;

            auto& tb = registry.get<Transform>(b);
            auto& rbb = registry.get<Rigidbody>(b);

            // POSITION SOLVE
            if (rba.IsStatic && rbb.IsStatic)
                continue;

            if (rba.IsStatic) {
                tb.LocalPosition += manifold.Normal * manifold.Penetration;
            } else if (rbb.IsStatic) {
                ta.LocalPosition -= manifold.Normal * manifold.Penetration;
            } else {
                ta.LocalPosition -= manifold.Normal * manifold.Penetration * 0.5f;
                tb.LocalPosition += manifold.Normal * manifold.Penetration * 0.5f;
            }

            // VELOCITY SOLVE - cancel each body's velocity component that
            // would deepen the penetration. manifold.Normal points from a
            // to b, so a's penetrating direction is +Normal and b's is -Normal.
            if (!rba.IsStatic) {
                float va = glm::dot(rba.Velocity, manifold.Normal);
                if (va > 0.0f)
                    rba.Velocity -= manifold.Normal * va;
            }

            if (!rbb.IsStatic) {
                float vb = glm::dot(rbb.Velocity, manifold.Normal);
                if (vb < 0.0f)
                    rbb.Velocity -= manifold.Normal * vb;
            }
        }
    }
}

} // namespace Wankel
