#include "wkpch.h"
#include "SphereMeshCollision.h"
#include "Triangle.h"

#include <limits>

namespace Wankel {

CollisionManifold SpherevsMesh(const Sphere& sphere, const glm::vec3& meshOrigin, const TriangleMesh& mesh) {
    CollisionManifold result;

    // 1. Whole-mesh reject: sphere's own AABB vs the mesh's world AABB -
    // avoids a per-triangle scan entirely for the common case where the
    // sphere isn't anywhere near this particular mesh/chunk.
    AABB meshWorldBounds {mesh.LocalBounds().Min + meshOrigin, mesh.LocalBounds().Max + meshOrigin};
    AABB sphereBounds = AABB::FromCenterHalfSize(sphere.Center, glm::vec3(sphere.Radius));
    if (!sphereBounds.Intersects(meshWorldBounds))
        return result;

    // 2. Track the globally closest point across every triangle.
    float bestDist2 = std::numeric_limits<float>::max();
    glm::vec3 bestClosestPoint {0.0f};

    size_t triCount = mesh.GetTriangleCount();
    for (size_t i = 0; i < triCount; i++) {
        glm::vec3 a, b, c;
        mesh.GetTriangle(i, a, b, c);
        a += meshOrigin;
        b += meshOrigin;
        c += meshOrigin;

        glm::vec3 cp = ClosestPointOnTriangle(sphere.Center, a, b, c);
        glm::vec3 probeDelta = sphere.Center - cp;
        float dist2 = glm::dot(probeDelta, probeDelta);

        if (dist2 < bestDist2) {
            bestDist2 = dist2;
            bestClosestPoint = cp;
        }
    }

    // 3. No collision
    if (bestDist2 > sphere.Radius * sphere.Radius)
        return result;

    // 4. Collision happened
    result.Colliding = true;

    float dist = sqrt(bestDist2);

    // 5. Normal + penetration. Mesh is always the hi-type in this pair
    // (ColliderType::Mesh is the highest enum value), so the manifold
    // normal must point lo->hi, i.e. from the sphere toward the mesh
    // surface: delta = closestPointOnTri - sphere.Center.
    if (dist > 0.0001f) {
        glm::vec3 delta = bestClosestPoint - sphere.Center;
        result.Normal = delta / dist;
        result.Penetration = sphere.Radius - dist;
    } else {
        // Sphere center lies exactly on the mesh surface.
        result.Normal = glm::vec3(0, 1, 0);
        result.Penetration = sphere.Radius;
    }

    return result;
}

} // namespace Wankel
