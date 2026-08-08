#pragma once

#include "../Collision/BroadPhase/SpatialHashGrid.h"

#include <glm/glm.hpp>

namespace Wankel {

class Scene;

// A constant per-frame force applied to every non-static Rigidbody (see PhysicsSystem::Update) -
// off by default so existing scenes/entities are unaffected until something opts in (e.g.
// WorldManager enabling it for the Overworld, disabling it for the Void).
struct GravitySettings {
    bool Enabled = false;
    float Magnitude = 9.81f;
    glm::vec3 Direction = {0.0f, -1.0f, 0.0f};

    // Hard cap on every non-static Rigidbody's speed (see PhysicsSystem::Update), applied regardless
    // of Enabled - position integration is plain discrete-time Euler with no substepping or swept
    // collision, so an unbounded fall (or any other unbounded velocity source) can accumulate enough
    // speed in one frame to skip clean over a thin collider ("tunneling"). Independent of and
    // complementary to MechtrixLayer's own dt clamp, which bounds the *step size* but not velocity
    // itself.
    float TerminalVelocity = 25.0f;
};

class PhysicsSystem {
public:
    void Update(Scene& scene, float dt);

    // Plain public settings struct, same convention as Renderer's FogSettings/LightSettings - read/
    // written directly (e.g. via Scene::GetGravitySettings()), not through getter/setter methods.
    GravitySettings Gravity;

    // Static colliders (terrain chunks, static level geometry) live in a broad-phase grid that's
    // rebuilt only when this is called, not every Update() - see Update()'s own comment for why.
    // Call after adding/removing/moving a static collider entity (e.g. terrain regeneration).
    void MarkStaticCollidersDirty() { m_StaticGridDirty = true; }

    // Incrementally adds/moves one static collider in the cached grid - O(cells that entity's
    // bounds span), not O(whole static collider set) like MarkStaticCollidersDirty's full rebuild.
    // Use this instead when only a handful of static colliders actually changed (e.g. voxel terrain
    // edits touching a few chunks out of a world that may have thousands).
    void UpdateStaticCollider(entt::entity entity, const AABB& worldBounds) {
        m_StaticGrid.Remove(entity);
        m_StaticGrid.InsertAABB(entity, worldBounds);
    }

    // Removes one static collider entity from the cached grid without touching any other entity -
    // pair to UpdateStaticCollider above for a destroyed static collider (e.g. an edited-away voxel
    // chunk).
    void RemoveStaticCollider(entt::entity entity) { m_StaticGrid.Remove(entity); }

private:
    void RebuildStaticGrid(Scene& scene);
    void RebuildDynamicGrid(Scene& scene);

    SpatialHashGrid m_DynamicGrid {1.0f}; // cell size ~ cube size, rebuilt every Update()

    // Static colliders (terrain chunks in particular) are typically an order of magnitude larger than
    // dynamic ones - a 1.0f cell size made InsertAABB/Remove span ~4900 cells for a single 16-unit
    // voxel chunk (see RebuildStaticGrid's own comment), turning any bulk add/remove (world regenerate,
    // entering/leaving the void) into millions of hash-map operations done synchronously in one frame.
    // 16.0f keeps a default-sized chunk to a handful of cells instead, at the cost of a slightly larger
    // (still cheap - static colliders are few relative to typical query volume) per-Query() candidate
    // list for anything using this grid.
    SpatialHashGrid m_StaticGrid {16.0f}; // cached across frames - see MarkStaticCollidersDirty()
    bool m_StaticGridDirty = true;        // starts dirty so the first Update() populates it
};

} // namespace Wankel
