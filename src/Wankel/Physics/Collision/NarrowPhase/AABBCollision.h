#pragma once
#include "../CollisionManifold.h"
#include "../BroadPhase/AABB.h"

namespace Wankel {

	CollisionManifold AABBvsAABB(const AABB& a, const AABB& b);

}
