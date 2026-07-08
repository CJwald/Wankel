#pragma once

#include <entt/entt.hpp>

#include "CollisionManifold.h"

namespace Wankel {

class Scene;

bool ResolveCollision(Scene& scene, entt::entity a, entt::entity b, CollisionManifold& outManifold);

} // namespace Wankel
