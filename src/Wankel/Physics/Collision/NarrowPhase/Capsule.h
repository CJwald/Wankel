#pragma once

#include <glm/glm.hpp>

namespace Wankel {

// Upright capsule: a vertical line segment of length 2*HalfHeight centered on
// Center, swept by Radius. Segment endpoints are Center +/- (0, HalfHeight, 0)
// - orientation is not supported (matches AABBCollider/SphereCollider's own
// translation-only Offset model; a capsule collider is always axis-aligned to
// world Y).
struct Capsule {
    glm::vec3 Center {0.0f};
    float Radius = 0.5f;
    float HalfHeight = 0.5f;

    glm::vec3 PointA() const { return Center - glm::vec3(0.0f, HalfHeight, 0.0f); }
    glm::vec3 PointB() const { return Center + glm::vec3(0.0f, HalfHeight, 0.0f); }
};

// Closest point on the segment [a, b] to point p (Ericson, Real-Time
// Collision Detection, 5.1.2).
inline glm::vec3 ClosestPointOnSegment(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b) {
    glm::vec3 ab = b - a;
    float abLen2 = glm::dot(ab, ab);

    if (abLen2 < 1e-8f)
        return a; // degenerate zero-length segment

    float t = glm::dot(p - a, ab) / abLen2;
    t = glm::clamp(t, 0.0f, 1.0f);

    return a + t * ab;
}

} // namespace Wankel
