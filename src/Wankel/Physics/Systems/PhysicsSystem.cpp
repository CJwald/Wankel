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

    auto buildAABBView = registry.view<Transform, AABBCollider>();

    for (auto e : buildAABBView) {
        auto& t = registry.get<Transform>(e);
        auto& c = registry.get<AABBCollider>(e);

        glm::vec3 center = t.LocalPosition + c.Offset;

        m_Grid.Insert(e, center);
    }

    // Sphere colliders must also be indexed, otherwise a sphere-vs-sphere
    // pair can never be discovered from either side (both spheres are
    // absent from the grid) even though sphere-vs-AABB and AABB-vs-AABB work.
    auto buildSphereView = registry.view<Transform, SphereCollider>();

    for (auto e : buildSphereView) {
        auto& t = registry.get<Transform>(e);
        auto& c = registry.get<SphereCollider>(e);

        glm::vec3 center = t.LocalPosition + c.Offset;

        m_Grid.Insert(e, center);
    }

    // COLLISION
    auto view = registry.view<Transform, Rigidbody>();

    // The broad-phase grid only contains entities with a collider
    // (AABBCollider or SphereCollider), so an entity with neither (only
    // Transform + Rigidbody) can discover a pair but never be discovered as
    // one. Track pairs already resolved this frame by canonical (min, max)
    // entity key so a symmetric discovery doesn't resolve the same pair
    // twice, without assuming every pair is discovered from both directions.
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

            // The broad-phase grid only requires Transform + a collider, so
            // b may be static level geometry with no Rigidbody at all (e.g.
            // a collider-only wall). Treat that as implicitly static rather
            // than asserting/UB on an unconditional get<Rigidbody>(b).
            auto* rbbPtr = registry.try_get<Rigidbody>(b);
            bool bIsStatic = !rbbPtr || rbbPtr->IsStatic;

            if (rba.IsStatic && bIsStatic)
                continue;

            // Inverse mass: 0 for a static/collider-only body (infinite
            // mass - it never moves or absorbs velocity from a collision),
            // otherwise 1/Mass. Clamp Mass away from <=0 so a misconfigured
            // Rigidbody can't divide-by-zero and inject NaN into position
            // or velocity (the same failure mode the normal-matrix fix
            // upstream guards against).
            constexpr float kMinMass = 0.0001f;
            float invMassA = rba.IsStatic ? 0.0f : 1.0f / glm::max(rba.Mass, kMinMass);
            float invMassB = bIsStatic ? 0.0f : 1.0f / glm::max(rbbPtr->Mass, kMinMass);
            float invMassSum = invMassA + invMassB;

            // POSITION SOLVE - split penetration correction proportional to
            // each body's inverse mass (heavier moves less). This is a
            // strict generalization of the old fixed-share split: it
            // reduces to "static side doesn't move, dynamic side takes the
            // full correction" when one side has infinite mass, and to the
            // old flat 50/50 split when both dynamic sides have equal mass
            // - only differently-massed dynamic pairs actually change
            // behavior, matching what this item set out to fix.
            ta.LocalPosition -= manifold.Normal * manifold.Penetration * (invMassA / invMassSum);
            tb.LocalPosition += manifold.Normal * manifold.Penetration * (invMassB / invMassSum);

            // VELOCITY SOLVE - single-contact normal impulse (no friction,
            // restitution 0 i.e. fully inelastic along the normal, matching
            // the previous "objects stop dead on contact" feel) distributed
            // by mass, instead of independently zeroing each body's own
            // penetrating velocity component regardless of what it hit.
            // The old approach wasn't actually momentum-conserving even for
            // equal masses (both bodies fully stopped rather than ending up
            // moving together); this is the standard textbook 2-body
            // impulse formula and is exact when one side is static.
            glm::vec3 velB = bIsStatic ? glm::vec3(0.0f) : rbbPtr->Velocity;
            glm::vec3 relativeVelocity = velB - rba.Velocity;
            float velAlongNormal = glm::dot(relativeVelocity, manifold.Normal);

            if (velAlongNormal < 0.0f) { // still closing; separating pairs need no resolution
                glm::vec3 impulse = manifold.Normal * (-velAlongNormal / invMassSum);

                if (!rba.IsStatic)
                    rba.Velocity -= impulse * invMassA;
                if (!bIsStatic)
                    rbbPtr->Velocity += impulse * invMassB;
            }
        }
    }
}

} // namespace Wankel
