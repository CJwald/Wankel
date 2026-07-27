#include "wkpch.h"
#include "CapsuleMeshCollision.h"
#include "Triangle.h"

#include <limits>

namespace Wankel {

CollisionManifold CapsulevsMesh(const Capsule& capsule, const glm::vec3& meshOrigin, const TriangleMesh& mesh) {
    CollisionManifold result;

    // 1. Whole-mesh reject: capsule's own AABB (segment extents +/- radius)
    // vs the mesh's world AABB.
    AABB meshWorldBounds {mesh.LocalBounds().Min + meshOrigin, mesh.LocalBounds().Max + meshOrigin};
    glm::vec3 segMin = glm::min(capsule.PointA(), capsule.PointB()) - glm::vec3(capsule.Radius);
    glm::vec3 segMax = glm::max(capsule.PointA(), capsule.PointB()) + glm::vec3(capsule.Radius);
    if (!AABB {segMin, segMax}.Intersects(meshWorldBounds))
        return result;

    glm::vec3 segA = capsule.PointA();
    glm::vec3 segB = capsule.PointB();

    float bestDist2 = std::numeric_limits<float>::max();
    glm::vec3 bestPointOnSeg {0.0f};
    glm::vec3 bestPointOnTri {0.0f};

    constexpr int kIterations = 4;

    size_t triCount = mesh.GetTriangleCount();
    for (size_t i = 0; i < triCount; i++) {
        glm::vec3 a, b, c;
        mesh.GetTriangle(i, a, b, c);
        a += meshOrigin;
        b += meshOrigin;
        c += meshOrigin;

        // Closest points between the segment and this triangle via
        // alternating projection - the same technique CapsulevsAABB uses
        // for segment-vs-box, just swapping the box-clamp step for a
        // ClosestPointOnTriangle call.
        glm::vec3 pointOnSeg = capsule.Center;
        glm::vec3 pointOnTri = pointOnSeg;

        for (int iter = 0; iter < kIterations; iter++) {
            pointOnTri = ClosestPointOnTriangle(pointOnSeg, a, b, c);
            pointOnSeg = ClosestPointOnSegment(pointOnTri, segA, segB);
        }

        glm::vec3 probeDelta = pointOnSeg - pointOnTri;
        float dist2 = glm::dot(probeDelta, probeDelta);

        if (dist2 < bestDist2) {
            bestDist2 = dist2;
            bestPointOnSeg = pointOnSeg;
            bestPointOnTri = pointOnTri;
        }
    }

    // 2. No collision
    if (bestDist2 > capsule.Radius * capsule.Radius)
        return result;

    // 3. Collision happened
    result.Colliding = true;

    float dist = sqrt(bestDist2);

    // 4. Normal + penetration. Mesh is always hi in this pair, so the
    // normal must point lo->hi: delta = pointOnTri - pointOnSeg.
    if (dist > 0.0001f) {
        glm::vec3 delta = bestPointOnTri - bestPointOnSeg;
        result.Normal = delta / dist;
        result.Penetration = capsule.Radius - dist;
    } else {
        // Segment passes through (or exactly touches) the mesh surface.
        result.Normal = glm::vec3(0, 1, 0);
        result.Penetration = capsule.Radius;
    }

    return result;
}

} // namespace Wankel
