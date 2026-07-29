#pragma once

#include "../CollisionManifold.h"
#include "Sphere.h"
#include "../TriangleMesh.h"

namespace Wankel {

// meshOrigin is the mesh entity's Transform::LocalPosition + MeshCollider::Offset
// (MeshCollider is translation-only, so this is the whole world transform).
CollisionManifold SpherevsMesh(const Sphere& sphere, const glm::vec3& meshOrigin, const TriangleMesh& mesh);

} // namespace Wankel
