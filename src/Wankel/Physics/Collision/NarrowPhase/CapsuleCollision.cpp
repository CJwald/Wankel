#include "wkpch.h"
#include "CapsuleCollision.h"

namespace Wankel {

namespace {

// Closest points between segments [p1,q1] and [p2,q2] (Ericson, Real-Time
// Collision Detection, 5.1.9). Handles parallel/degenerate segments via the
// standard clamp-and-reclamp case analysis.
void ClosestPtSegmentSegment(const glm::vec3& p1, const glm::vec3& q1, const glm::vec3& p2, const glm::vec3& q2,
                              glm::vec3& c1, glm::vec3& c2) {
    constexpr float kEpsilon = 1e-8f;

    glm::vec3 d1 = q1 - p1;
    glm::vec3 d2 = q2 - p2;
    glm::vec3 r = p1 - p2;

    float a = glm::dot(d1, d1);
    float e = glm::dot(d2, d2);
    float f = glm::dot(d2, r);

    float s, t;

    if (a <= kEpsilon && e <= kEpsilon) {
        // Both segments degenerate into points
        c1 = p1;
        c2 = p2;
        return;
    }

    if (a <= kEpsilon) {
        // First segment degenerates into a point
        s = 0.0f;
        t = glm::clamp(f / e, 0.0f, 1.0f);
    } else {
        float c = glm::dot(d1, r);

        if (e <= kEpsilon) {
            // Second segment degenerates into a point
            t = 0.0f;
            s = glm::clamp(-c / a, 0.0f, 1.0f);
        } else {
            float b = glm::dot(d1, d2);
            float denom = a * e - b * b;

            s = (denom != 0.0f) ? glm::clamp((b * f - c * e) / denom, 0.0f, 1.0f) : 0.0f;
            t = (b * s + f) / e;

            if (t < 0.0f) {
                t = 0.0f;
                s = glm::clamp(-c / a, 0.0f, 1.0f);
            } else if (t > 1.0f) {
                t = 1.0f;
                s = glm::clamp((b - c) / a, 0.0f, 1.0f);
            }
        }
    }

    c1 = p1 + d1 * s;
    c2 = p2 + d2 * t;
}

} // namespace

CollisionManifold CapsulevsCapsule(const Capsule& a, const Capsule& b) {
    CollisionManifold result;

    glm::vec3 closestOnA, closestOnB;
    ClosestPtSegmentSegment(a.PointA(), a.PointB(), b.PointA(), b.PointB(), closestOnA, closestOnB);

    // Same-type pair (like AABBvsAABB/SpherevsSphere) - normal points a->b.
    glm::vec3 delta = closestOnB - closestOnA;

    float dist2 = glm::dot(delta, delta);
    float radiusSum = a.Radius + b.Radius;

    if (dist2 > radiusSum * radiusSum)
        return result;

    result.Colliding = true;

    float dist = sqrt(dist2);

    if (dist > 0.0001f) {
        result.Normal = delta / dist;
        result.Penetration = radiusSum - dist;
    } else {
        // Segments' closest points coincide (e.g. crossing/overlapping cores).
        result.Normal = glm::vec3(0, 1, 0);
        result.Penetration = radiusSum;
    }

    return result;
}

} // namespace Wankel
