#pragma once

#include "../CollisionManifold.h"
#include "Capsule.h"
#include "../BroadPhase/AABB.h"

namespace Wankel {

CollisionManifold CapsulevsAABB(const Capsule& capsule, const AABB& aabb);

}
