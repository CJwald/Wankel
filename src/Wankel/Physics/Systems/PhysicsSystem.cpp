#include "wkpch.h"
#include "PhysicsSystem.h"

#include <Wankel/ECS/Scene.h>
#include <Wankel/ECS/Components.h>

#include "../Collision/BroadPhase/AABB.h"
#include "../Collision/CollisionDispatcher.h"

namespace Wankel {

namespace {

bool IsStaticEntity(entt::registry& registry, entt::entity e) {
    auto* rb = registry.try_get<Rigidbody>(e);
    return !rb || rb->IsStatic;
}

} // namespace

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

            // Once there's no vertical input, hand vertical velocity fully to gravity (below) instead
            // of Movement decelerating it back toward 0 - see PlayerController::FlightGravityScale /
            // Rigidbody::GravityScale. Untouched whenever gravity doesn't apply to this entity (e.g.
            // Flight mode at its default scale, or gravity-less worlds), so non-player Movement users
            // and Ctrl-descend in the Void are unaffected.
            bool gravityOwnsVertical = Gravity.Enabled && rb.GravityScale > 0.0f && glm::abs(m.MoveIntent.y) < 0.001f;
            if (gravityOwnsVertical)
                deltaVel.y = 0.0f;

            float deltaMag = glm::length(deltaVel);

            float maxDV = accel * dt;

            if (deltaMag > maxDV) {
                deltaVel = glm::normalize(deltaVel) * maxDV;
            }

            rb.Velocity += deltaVel;
        }
    }

    // Gravity - a constant force on every non-static Rigidbody, regardless of whether it has a
    // Movement component. Note: an entity with *both* Rigidbody and Movement (the player, today) has
    // its vertical velocity re-corrected toward Movement's own target almost every frame (Movement's
    // Acceleration/Deceleration are far larger than any reasonable gravity magnitude), so gravity
    // mostly cancels out for it - real player falling needs Movement's own model to stop treating all
    // axes as fully player-controlled (see docs/TODO.md item 6), not just this force existing.
    if (Gravity.Enabled) {
        auto view = registry.view<Rigidbody>();

        for (auto e : view) {
            auto& rb = registry.get<Rigidbody>(e);

            if (rb.IsStatic)
                continue;

            rb.Velocity += Gravity.Direction * Gravity.Magnitude * rb.GravityScale * dt;
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

    // BUILD SPATIAL GRIDS
    //
    // Static colliders (terrain chunks in particular) almost never move or change, but
    // InsertAABB() spans every grid cell a collider's bounds touch - even at this grid's coarser,
    // static-tuned cell size (see m_StaticGrid's own comment), a full-world voxel terrain is still
    // thousands of colliders. Re-inserting every static collider from scratch on every single
    // Update() call was costing a full-world voxel terrain multiple *million* hash-map insertions a
    // frame for geometry that hadn't changed since the previous frame. The static grid is now cached
    // across frames and only rebuilt when MarkStaticCollidersDirty() says the static collider set
    // actually changed (terrain regenerated, a static prop spawned/despawned, etc) - the dynamic grid
    // (few, moving bodies) is cheap enough to still just rebuild every frame.
    if (m_StaticGridDirty) {
        RebuildStaticGrid(scene);
        m_StaticGridDirty = false;
    }
    RebuildDynamicGrid(scene);

    // COLLISION
    auto view = registry.view<Transform, Rigidbody>();

    // The broad-phase grids only contain entities with a collider
    // (AABBCollider, SphereCollider, CapsuleCollider, or MeshCollider), so
    // an entity with none of those (only Transform + Rigidbody) can
    // discover a pair but never be discovered as one. Track pairs already
    // resolved this frame by canonical (min, max)
    // entity key so a symmetric discovery doesn't resolve the same pair
    // twice, without assuming every pair is discovered from both directions.
    std::unordered_set<uint64_t> resolvedPairs;

    for (auto a : view) {
        auto& ta = registry.get<Transform>(a);
        auto& rba = registry.get<Rigidbody>(a);

        auto candidates = m_StaticGrid.Query(ta.LocalPosition);
        auto dynamicCandidates = m_DynamicGrid.Query(ta.LocalPosition);
        candidates.insert(candidates.end(), dynamicCandidates.begin(), dynamicCandidates.end());

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

            // VELOCITY SOLVE - single-contact normal impulse (restitution 0
            // i.e. fully inelastic along the normal, matching the previous
            // "objects stop dead on contact" feel) distributed by mass,
            // instead of independently zeroing each body's own penetrating
            // velocity component regardless of what it hit. The old approach
            // wasn't actually momentum-conserving even for equal masses (both
            // bodies fully stopped rather than ending up moving together);
            // this is the standard textbook 2-body impulse formula and is
            // exact when one side is static. A tangential Coulomb friction
            // impulse (see below) now follows the same pattern.
            glm::vec3 velB = bIsStatic ? glm::vec3(0.0f) : rbbPtr->Velocity;
            glm::vec3 relativeVelocity = velB - rba.Velocity;
            float velAlongNormal = glm::dot(relativeVelocity, manifold.Normal);

            if (velAlongNormal < 0.0f) { // still closing; separating pairs need no resolution
                float normalImpulseMag = -velAlongNormal / invMassSum;
                glm::vec3 impulse = manifold.Normal * normalImpulseMag;

                if (!rba.IsStatic)
                    rba.Velocity -= impulse * invMassA;
                if (!bIsStatic)
                    rbbPtr->Velocity += impulse * invMassB;

                // FRICTION - tangential (in-surface) component of relative velocity, opposed up to
                // the Coulomb limit (coefficient * normal impulse magnitude) rather than fully
                // canceled outright - that clamp is exactly what lets a steep-enough slope still let
                // a body slide (gravity's along-slope component can exceed available friction) while
                // a shallow slope's smaller tangential velocity gets fully arrested. manifold.Friction
                // is the geometric mean of both colliders' own Friction (see
                // CollisionDispatcher::ResolveCollision).
                glm::vec3 tangentVelocity = relativeVelocity - velAlongNormal * manifold.Normal;
                float tangentSpeed = glm::length(tangentVelocity);

                if (tangentSpeed > 1e-5f) {
                    glm::vec3 tangent = tangentVelocity / tangentSpeed;
                    float tangentialVelAlongTangent = glm::dot(relativeVelocity, tangent);

                    float maxFriction = manifold.Friction * normalImpulseMag;
                    float frictionImpulseMag =
                        glm::clamp(-tangentialVelAlongTangent / invMassSum, -maxFriction, maxFriction);
                    glm::vec3 frictionImpulse = tangent * frictionImpulseMag;

                    if (!rba.IsStatic)
                        rba.Velocity -= frictionImpulse * invMassA;
                    if (!bIsStatic)
                        rbbPtr->Velocity += frictionImpulse * invMassB;
                }
            }
        }
    }
}

// Indexes every collider entity whose static-ness (see IsStaticEntity) matches this grid's
// role - same 4 collider-type blocks (and same reasons each needs its own Insert/InsertAABB
// pass) as before the static/dynamic split, just gated by IsStaticEntity so a given entity ends
// up in exactly one of the two grids.
namespace {

void BuildGrid(entt::registry& registry, SpatialHashGrid& grid, bool wantStatic) {
    auto aabbView = registry.view<Transform, AABBCollider>();
    for (auto e : aabbView) {
        if (IsStaticEntity(registry, e) != wantStatic)
            continue;
        auto& t = registry.get<Transform>(e);
        auto& c = registry.get<AABBCollider>(e);
        grid.Insert(e, t.LocalPosition + c.Offset);
    }

    // Sphere colliders must also be indexed, otherwise a sphere-vs-sphere
    // pair can never be discovered from either side (both spheres are
    // absent from the grid) even though sphere-vs-AABB and AABB-vs-AABB work.
    auto sphereView = registry.view<Transform, SphereCollider>();
    for (auto e : sphereView) {
        if (IsStaticEntity(registry, e) != wantStatic)
            continue;
        auto& t = registry.get<Transform>(e);
        auto& c = registry.get<SphereCollider>(e);
        grid.Insert(e, t.LocalPosition + c.Offset);
    }

    // Same reasoning as sphere colliders above - capsules must be indexed
    // too, otherwise pairs involving a capsule are never discovered.
    auto capsuleView = registry.view<Transform, CapsuleCollider>();
    for (auto e : capsuleView) {
        if (IsStaticEntity(registry, e) != wantStatic)
            continue;
        auto& t = registry.get<Transform>(e);
        auto& c = registry.get<CapsuleCollider>(e);
        grid.Insert(e, t.LocalPosition + c.Offset);
    }

    // Mesh colliders (static terrain) are usually much larger than one grid
    // cell, so a single center-point Insert() would make them undiscoverable
    // from most nearby dynamic bodies - InsertAABB() spans every cell the
    // mesh's world bounds actually touch instead.
    auto meshView = registry.view<Transform, MeshCollider>();
    for (auto e : meshView) {
        if (IsStaticEntity(registry, e) != wantStatic)
            continue;
        auto& t = registry.get<Transform>(e);
        auto& c = registry.get<MeshCollider>(e);
        if (!c.Mesh)
            continue;
        glm::vec3 origin = t.LocalPosition + c.Offset;
        AABB worldBounds {c.Mesh->LocalBounds().Min + origin, c.Mesh->LocalBounds().Max + origin};
        grid.InsertAABB(e, worldBounds);
    }
}

} // namespace

void PhysicsSystem::RebuildStaticGrid(Scene& scene) {
    m_StaticGrid.Clear();
    BuildGrid(scene.Registry(), m_StaticGrid, /*wantStatic=*/true);
}

void PhysicsSystem::RebuildDynamicGrid(Scene& scene) {
    m_DynamicGrid.Clear();
    BuildGrid(scene.Registry(), m_DynamicGrid, /*wantStatic=*/false);
}

} // namespace Wankel
