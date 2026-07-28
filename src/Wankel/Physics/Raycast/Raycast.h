#pragma once

#include "Ray.h"
#include "RaycastHit.h"

namespace Wankel {

class Scene;
struct Sphere;

bool IntersectRaySphere(const Ray& ray, const Sphere& sphere, float& outDistance);

bool RaycastAABB(Scene& scene, const Ray& ray, RaycastHit& outHit, float maxDistance = 1000.0f);

// Only tests MeshCollider entities (voxel terrain chunks) - AABBCollider/SphereCollider/
// CapsuleCollider entities (player, enemies, ...) are structurally invisible to this, the same way
// RaycastAABB above only ever sees AABBCollider entities.
bool RaycastMesh(Scene& scene, const Ray& ray, RaycastHit& outHit, float maxDistance = 1000.0f);

} // namespace Wankel
