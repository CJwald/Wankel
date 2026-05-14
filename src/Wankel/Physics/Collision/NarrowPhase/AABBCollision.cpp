#include "AABBCollision.h"

namespace Wankel {

CollisionManifold AABBvsAABB(const AABB& a, const AABB& b) {
    CollisionManifold m;

    glm::vec3 aCenter = (a.Min + a.Max) * 0.5f;
    glm::vec3 bCenter = (b.Min + b.Max) * 0.5f;

    glm::vec3 delta = bCenter - aCenter;

    glm::vec3 overlap = glm::vec3(
        (a.Max.x - b.Min.x) < (b.Max.x - a.Min.x) ? (a.Max.x - b.Min.x) : (b.Max.x - a.Min.x),
        (a.Max.y - b.Min.y) < (b.Max.y - a.Min.y) ? (a.Max.y - b.Min.y) : (b.Max.y - a.Min.y),
        (a.Max.z - b.Min.z) < (b.Max.z - a.Min.z) ? (a.Max.z - b.Min.z) : (b.Max.z - a.Min.z)
    );

    if (overlap.x <= 0 || overlap.y <= 0 || overlap.z <= 0)
        return m;

    m.Colliding = true;

    // pick smallest penetration axis
    if (overlap.x < overlap.y && overlap.x < overlap.z) {
        m.Penetration = overlap.x;
        m.Normal = glm::vec3((delta.x < 0) ? -1 : 1, 0, 0);
    }
    else if (overlap.y < overlap.z) {
        m.Penetration = overlap.y;
        m.Normal = glm::vec3(0, (delta.y < 0) ? -1 : 1, 0);
    }
    else {
        m.Penetration = overlap.z;
        m.Normal = glm::vec3(0, 0, (delta.z < 0) ? -1 : 1);
    }

    return m;
}

}
