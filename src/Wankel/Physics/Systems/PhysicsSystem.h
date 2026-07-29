#pragma once

#include "../Collision/BroadPhase/SpatialHashGrid.h"

namespace Wankel {

class Scene;

class PhysicsSystem {
public:
    void Update(Scene& scene, float dt);

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
    SpatialHashGrid m_StaticGrid {1.0f};  // cached across frames - see MarkStaticCollidersDirty()
    bool m_StaticGridDirty = true;        // starts dirty so the first Update() populates it
};

} // namespace Wankel
