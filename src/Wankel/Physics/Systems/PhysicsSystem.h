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

private:
    void RebuildStaticGrid(Scene& scene);
    void RebuildDynamicGrid(Scene& scene);

    SpatialHashGrid m_DynamicGrid {1.0f}; // cell size ~ cube size, rebuilt every Update()
    SpatialHashGrid m_StaticGrid {1.0f};  // cached across frames - see MarkStaticCollidersDirty()
    bool m_StaticGridDirty = true;        // starts dirty so the first Update() populates it
};

} // namespace Wankel
