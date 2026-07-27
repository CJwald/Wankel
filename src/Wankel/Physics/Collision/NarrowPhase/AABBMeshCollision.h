#pragma once

#include "../CollisionManifold.h"
#include "../BroadPhase/AABB.h"
#include "../TriangleMesh.h"

namespace Wankel {

CollisionManifold AABBvsMesh(const AABB& aabb, const glm::vec3& meshOrigin, const TriangleMesh& mesh);

}
