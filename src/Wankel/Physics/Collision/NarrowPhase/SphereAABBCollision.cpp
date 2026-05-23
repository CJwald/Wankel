#include "wkpch.h"
#include "SphereAABBCollision.h"

namespace Wankel {

CollisionManifold SpherevsAABB(const Sphere& sphere, const AABB& aabb) {
    CollisionManifold result;

    // 1. Find closest point on AABB to sphere center
    glm::vec3 closestPoint;

    closestPoint.x = glm::clamp(sphere.Center.x, aabb.Min.x, aabb.Max.x);

    closestPoint.y = glm::clamp(sphere.Center.y, aabb.Min.y, aabb.Max.y);

    closestPoint.z = glm::clamp(sphere.Center.z, aabb.Min.z, aabb.Max.z);

    // 2. Vector from closest point to sphere center
    glm::vec3 delta = sphere.Center - closestPoint;

    float dist2 = glm::dot(delta, delta);
    float radius2 = sphere.Radius * sphere.Radius;

    // 3. No collision
    if (dist2 > radius2)
        return result;

    // 4. Collision happened
    result.Colliding = true;

    float dist = sqrt(dist2);

    // 5. Normal + penetration
    if (dist > 0.00001f) {
        result.Normal = delta / dist;
        result.Penetration = sphere.Radius - dist;
    }
    else {
        // sphere center is inside AABB or exactly on edge
        result.Normal = glm::vec3(0, 1, 0);
        result.Penetration = sphere.Radius;
    }

    return result;
}

}
