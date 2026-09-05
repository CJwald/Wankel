#pragma once

#include "Ray.h"
#include "RaycastHit.h"

#include <entt/entt.hpp>

namespace Wankel {

class Scene;
struct Sphere;

bool IntersectRaySphere(const Ray& ray, const Sphere& sphere, float& outDistance);

bool RaycastAABB(Scene& scene, const Ray& ray, RaycastHit& outHit, float maxDistance = 1000.0f);

// Only tests MeshCollider entities (voxel terrain chunks) - AABBCollider/SphereCollider/
// CapsuleCollider entities (player, enemies, ...) are structurally invisible to this, the same way
// RaycastAABB above only ever sees AABBCollider entities.
bool RaycastMesh(Scene& scene, const Ray& ray, RaycastHit& outHit, float maxDistance = 1000.0f);

// Only tests SphereCollider entities (player, enemies, ...). excludeEntity skips one entity outright
// (e.g. the shooter, so a weapon's own fire raycast can't hit itself).
bool RaycastSphere(Scene& scene, const Ray& ray, RaycastHit& outHit, float maxDistance = 1000.0f,
                   entt::entity excludeEntity = entt::null);

// Dynamic-entity counterpart to RaycastMesh (which is terrain-only): tests every AABBCollider,
// SphereCollider, and CapsuleCollider entity in one pass and returns the closest hit across all
// three, so a caller doesn't need to know/guess which concrete shape a target entity has. Use this
// instead of RaycastSphere/RaycastAABB individually wherever the target could be any collider type
// (e.g. weapon hit-detection against mobs, whose collider type can be switched via the debug
// Inspector). excludeEntity skips one entity outright, same as RaycastSphere.
bool RaycastDynamicColliders(Scene& scene, const Ray& ray, RaycastHit& outHit, float maxDistance = 1000.0f,
                             entt::entity excludeEntity = entt::null);

} // namespace Wankel
