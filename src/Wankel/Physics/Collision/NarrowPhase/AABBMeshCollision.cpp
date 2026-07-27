#include "wkpch.h"
#include "AABBMeshCollision.h"

#include <limits>
#include <algorithm>
#include <cmath>

namespace Wankel {

namespace {

// Tests a single candidate separating axis: projects the box's half-extents
// and the triangle's 3 (box-relative) vertices onto `axis`, returns true if
// this axis separates them (i.e. no overlap on this axis alone).
bool IsSeparatingAxis(const glm::vec3& axis, const glm::vec3& boxHalfSize, const glm::vec3& v0, const glm::vec3& v1,
                     const glm::vec3& v2) {
    float axisLen2 = glm::dot(axis, axis);
    if (axisLen2 < 1e-10f)
        return false; // degenerate (near-zero) axis - can't separate, skip it

    float p0 = glm::dot(v0, axis);
    float p1 = glm::dot(v1, axis);
    float p2 = glm::dot(v2, axis);

    float r = boxHalfSize.x * fabsf(axis.x) + boxHalfSize.y * fabsf(axis.y) + boxHalfSize.z * fabsf(axis.z);

    float minP = std::min({p0, p1, p2});
    float maxP = std::max({p0, p1, p2});

    return minP > r || maxP < -r;
}

// Akenine-Moller "Fast 3D Triangle-Box Overlap Testing" (2001) - 13-axis
// SAT: 3 box-face normals, 1 triangle-face normal, 9 box-edge x
// triangle-edge cross products. This is the only one of the 3 new
// *vsMesh narrow-phase functions that needs a separate yes/no overlap
// predicate: Sphere/Capsule's own distance<=radius check already doubles
// as one (their radius buffers a near-miss), but AABB has no radius, so
// there's no distance-based formula to lean on instead - a real
// separating-axis test is structurally required, not just extra caution.
bool TriangleIntersectsAABB(const AABB& aabb, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    glm::vec3 boxCenter = (aabb.Min + aabb.Max) * 0.5f;
    glm::vec3 boxHalfSize = (aabb.Max - aabb.Min) * 0.5f;

    glm::vec3 v0 = a - boxCenter;
    glm::vec3 v1 = b - boxCenter;
    glm::vec3 v2 = c - boxCenter;

    glm::vec3 f0 = v1 - v0;
    glm::vec3 f1 = v2 - v1;
    glm::vec3 f2 = v0 - v2;

    const glm::vec3 boxAxes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const glm::vec3 edges[3] = {f0, f1, f2};

    // 9 cross-product axes (box edge x triangle edge)
    for (const auto& boxAxis : boxAxes) {
        for (const auto& edge : edges) {
            if (IsSeparatingAxis(glm::cross(boxAxis, edge), boxHalfSize, v0, v1, v2))
                return false;
        }
    }

    // 3 box-face normal axes
    for (const auto& boxAxis : boxAxes) {
        if (IsSeparatingAxis(boxAxis, boxHalfSize, v0, v1, v2))
            return false;
    }

    // 1 triangle-face normal axis
    glm::vec3 triNormal = glm::cross(f0, f1);
    if (IsSeparatingAxis(triNormal, boxHalfSize, v0, v1, v2))
        return false;

    return true; // no separating axis found - overlapping
}

} // namespace

CollisionManifold AABBvsMesh(const AABB& aabb, const glm::vec3& meshOrigin, const TriangleMesh& mesh) {
    CollisionManifold result;

    // 1. Whole-mesh reject.
    AABB meshWorldBounds {mesh.LocalBounds().Min + meshOrigin, mesh.LocalBounds().Max + meshOrigin};
    if (!aabb.Intersects(meshWorldBounds))
        return result;

    glm::vec3 aabbCenter = (aabb.Min + aabb.Max) * 0.5f;
    glm::vec3 boxCorners[8];
    for (int i = 0; i < 8; i++) {
        boxCorners[i] = glm::vec3((i & 1) ? aabb.Max.x : aabb.Min.x, (i & 2) ? aabb.Max.y : aabb.Min.y,
                                  (i & 4) ? aabb.Max.z : aabb.Min.z);
    }

    bool anyOverlapping = false;
    float bestPenetration = 0.0f;
    glm::vec3 bestNormal {0.0f, 1.0f, 0.0f};

    size_t triCount = mesh.GetTriangleCount();
    for (size_t i = 0; i < triCount; i++) {
        glm::vec3 a, b, c;
        mesh.GetTriangle(i, a, b, c);
        a += meshOrigin;
        b += meshOrigin;
        c += meshOrigin;

        // 2. Correctness-first overlap predicate (see TriangleIntersectsAABB's
        // own comment for why this is required specifically for AABB).
        if (!TriangleIntersectsAABB(aabb, a, b, c))
            continue;

        anyOverlapping = true;

        // 3. Response: use the triangle's own (flat) face plane rather than
        // a closest-point search - a closest-point-to-triangle search
        // collapses to zero the instant the triangle's closest point falls
        // inside the box's volume, which happens for *any* nonzero overlap
        // depth (not just deep embedding), unlike a segment or a point
        // which always have a meaningful "closest point" distinct from a
        // containing box. The face-plane distance doesn't have that
        // singularity and gives a real depth for genuine overlap. This is
        // a documented simplification (an infinite-plane distance, not
        // aware of the triangle's finite edges/corners) - same spirit as
        // SpherevsAABB's own already-accepted naive degenerate fallback,
        // traded off against never reporting a bogus zero penetration for
        // a real overlap.
        glm::vec3 edge1 = b - a;
        glm::vec3 edge2 = c - a;
        float normalLen2 = glm::dot(glm::cross(edge1, edge2), glm::cross(edge1, edge2));
        if (normalLen2 < 1e-12f)
            continue; // degenerate (zero-area) triangle - skip

        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        // Orient normal to point from the box toward the mesh (lo->hi,
        // matching every other *vsMesh function's convention): the box's
        // center should be on the -normal side.
        if (glm::dot(normal, aabbCenter - a) > 0.0f)
            normal = -normal;

        // Deepest embedded corner along `normal` gives the penetration depth.
        float maxSignedDist = -std::numeric_limits<float>::max();
        for (const auto& corner : boxCorners)
            maxSignedDist = std::max(maxSignedDist, glm::dot(corner - a, normal));

        float penetration = std::max(maxSignedDist, 0.0f);

        if (penetration > bestPenetration) {
            bestPenetration = penetration;
            bestNormal = normal;
        }
    }

    // 4. No collision
    if (!anyOverlapping)
        return result;

    result.Colliding = true;
    result.Normal = bestNormal;
    result.Penetration = bestPenetration;

    return result;
}

} // namespace Wankel
