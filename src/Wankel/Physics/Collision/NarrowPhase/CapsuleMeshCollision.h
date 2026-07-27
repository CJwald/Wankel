#pragma once

#include "../CollisionManifold.h"
#include "Capsule.h"
#include "../TriangleMesh.h"

namespace Wankel {

CollisionManifold CapsulevsMesh(const Capsule& capsule, const glm::vec3& meshOrigin, const TriangleMesh& mesh);

}
