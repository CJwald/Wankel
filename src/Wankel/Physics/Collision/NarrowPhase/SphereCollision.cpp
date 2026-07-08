#include "wkpch.h"

#include "SphereCollision.h"

namespace Wankel {

CollisionManifold SpherevsSphere(const Sphere& a, const Sphere& b) {
    CollisionManifold result;

    glm::vec3 delta = b.Center - a.Center;
    float dist2 = glm::dot(delta, delta);
    float radiusSum = a.Radius + b.Radius;

    if (dist2 > radiusSum * radiusSum)
        return result;

    float dist = sqrt(dist2);
    result.Colliding = true;

    if (dist > 0.0001f) {
        result.Normal = delta / dist;
        result.Penetration = radiusSum - dist;
    } else {
        result.Normal = glm::vec3(0, 1, 0);
        result.Penetration = radiusSum;
    }

    return result;
}

} // namespace Wankel
