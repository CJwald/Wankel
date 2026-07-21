#pragma once

namespace Wankel {
class Scene;
}

// Builds the enemy rig: root entity (collider/physics), body, and 4 legs,
// each parented to the root. No animation/material - matches the current
// static-rig behavior exactly. Nothing outside this factory reuses the
// enemy's entity handle, so unlike PlayerFactory::Create this returns void.
class EnemyFactory {
public:
    static void Create(Wankel::Scene& scene);
};
