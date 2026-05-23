#pragma once

#include "../CollisionManifold.h"
#include "Sphere.h"
#include "../BroadPhase/AABB.h"

namespace Wankel {

CollisionManifold SpherevsAABB(
    const Sphere& sphere,
    const AABB& aabb);

}
