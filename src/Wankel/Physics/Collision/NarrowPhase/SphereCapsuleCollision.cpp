#include "wkpch.h"
#include "SphereCapsuleCollision.h"

namespace Wankel {

CollisionManifold SpherevsCapsule(const Sphere& sphere, const Capsule& capsule) {
    CollisionManifold result;

    // 1. Closest point on the capsule's core segment to the sphere center
    glm::vec3 closestPoint = ClosestPointOnSegment(sphere.Center, capsule.PointA(), capsule.PointB());

    // 2. Vector from sphere center to that point (Sphere is the lo-type
    // here, Capsule is hi - the manifold normal must point lo->hi, i.e.
    // sphere-surface toward the capsule, matching SpherevsAABB's own
    // lo(AABB)->hi(Sphere) convention).
    glm::vec3 delta = closestPoint - sphere.Center;

    float dist2 = glm::dot(delta, delta);
    float radiusSum = sphere.Radius + capsule.Radius;

    // 3. No collision
    if (dist2 > radiusSum * radiusSum)
        return result;

    // 4. Collision happened
    result.Colliding = true;

    float dist = sqrt(dist2);

    // 5. Normal + penetration
    if (dist > 0.0001f) {
        result.Normal = delta / dist;
        result.Penetration = radiusSum - dist;
    } else {
        // Sphere center lies exactly on the capsule's core segment.
        result.Normal = glm::vec3(0, 1, 0);
        result.Penetration = radiusSum;
    }

    return result;
}

} // namespace Wankel
