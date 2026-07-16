#include "wkpch.h"
#include "CapsuleAABBCollision.h"

namespace Wankel {

CollisionManifold CapsulevsAABB(const Capsule& capsule, const AABB& aabb) {
    CollisionManifold result;

    glm::vec3 segA = capsule.PointA();
    glm::vec3 segB = capsule.PointB();

    // Closest points between the segment and the box via alternating
    // projection: clamp the current segment guess onto the box, then
    // re-project that box point back onto the segment, and repeat. Both
    // sub-projections are exact (point-onto-AABB is a per-axis clamp,
    // point-onto-segment is ClosestPointOnSegment), and alternating
    // projection onto two convex sets converges to the true closest pair -
    // a handful of iterations is enough for a segment/box pair (no
    // oscillation risk like non-convex shapes could have).
    glm::vec3 pointOnSeg = capsule.Center;
    glm::vec3 pointOnBox = pointOnSeg;

    constexpr int kIterations = 4;
    for (int i = 0; i < kIterations; i++) {
        pointOnBox.x = glm::clamp(pointOnSeg.x, aabb.Min.x, aabb.Max.x);
        pointOnBox.y = glm::clamp(pointOnSeg.y, aabb.Min.y, aabb.Max.y);
        pointOnBox.z = glm::clamp(pointOnSeg.z, aabb.Min.z, aabb.Max.z);

        pointOnSeg = ClosestPointOnSegment(pointOnBox, segA, segB);
    }

    // AABB is the lo-type here, Capsule is hi - normal must point lo->hi,
    // i.e. from the box toward the capsule's segment (matching
    // SpherevsAABB's own lo(AABB)->hi(Sphere) convention).
    glm::vec3 delta = pointOnSeg - pointOnBox;

    float dist2 = glm::dot(delta, delta);
    float radius2 = capsule.Radius * capsule.Radius;

    if (dist2 > radius2)
        return result;

    result.Colliding = true;

    float dist = sqrt(dist2);

    if (dist > 0.0001f) {
        result.Normal = delta / dist;
        result.Penetration = capsule.Radius - dist;
    } else {
        // Segment passes through (or exactly touches) the box.
        result.Normal = glm::vec3(0, 1, 0);
        result.Penetration = capsule.Radius;
    }

    return result;
}

} // namespace Wankel
