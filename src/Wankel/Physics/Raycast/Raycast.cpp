#include "wkpch.h"

#include "Raycast.h"

#include <Wankel/ECS/Scene.h>
#include <Wankel/ECS/Components.h>

#include "../Collision/BroadPhase/AABB.h"
#include "../Collision/NarrowPhase/Capsule.h"
#include "../Collision/NarrowPhase/Sphere.h"
#include "../Collision/TriangleMesh.h"

namespace Wankel {

bool IntersectRaySphere(const Ray& ray, const Sphere& sphere, float& outDistance) {
    glm::vec3 oc = ray.Origin - sphere.Center;
    glm::vec3 dir = glm::normalize(ray.Direction);

    float a = glm::dot(dir, dir);
    float b = 2.0f * glm::dot(oc, dir);
    float c = glm::dot(oc, oc) - sphere.Radius * sphere.Radius;

    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f)
        return false;

    float sqrtDisc = sqrt(discriminant);

    float t0 = (-b - sqrtDisc) / (2.0f * a);
    float t1 = (-b + sqrtDisc) / (2.0f * a);
    float t = t0;

    if (t < 0.0f)
        t = t1;

    if (t < 0.0f)
        return false;

    outDistance = t;

    return true;
}


bool IntersectRayAABB(const Ray& ray, const AABB& aabb, float& t, glm::vec3& outNormal) {
    glm::vec3 dir = glm::normalize(ray.Direction);

    // Avoid dividing by exactly zero on axis-aligned rays (which would
    // otherwise risk a 0 * inf -> NaN slab test below).
    constexpr float kEpsilon = 1e-6f;
    if (std::abs(dir.x) < kEpsilon)
        dir.x = std::copysign(kEpsilon, dir.x);
    if (std::abs(dir.y) < kEpsilon)
        dir.y = std::copysign(kEpsilon, dir.y);
    if (std::abs(dir.z) < kEpsilon)
        dir.z = std::copysign(kEpsilon, dir.z);

    glm::vec3 invDir = 1.0f / dir;

    glm::vec3 t0 = (aabb.Min - ray.Origin) * invDir;
    glm::vec3 t1 = (aabb.Max - ray.Origin) * invDir;

    glm::vec3 tmin = glm::min(t0, t1);
    glm::vec3 tmax = glm::max(t0, t1);

    float nearT = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
    float farT = glm::min(glm::min(tmax.x, tmax.y), tmax.z);

    if (nearT > farT)
        return false;

    if (farT < 0.0f)
        return false;

    t = nearT;

    if (nearT == tmin.x)
        outNormal = glm::vec3(dir.x < 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
    else if (nearT == tmin.y)
        outNormal = glm::vec3(0.0f, dir.y < 0.0f ? 1.0f : -1.0f, 0.0f);
    else
        outNormal = glm::vec3(0.0f, 0.0f, dir.z < 0.0f ? 1.0f : -1.0f);

    return true;
}

bool RaycastAABB(Scene& scene, const Ray& ray, RaycastHit& outHit, float maxDistance) {
    auto& registry = scene.Registry();
    auto view = registry.view<Transform, AABBCollider>();

    bool hitAnything = false;
    float closest = maxDistance;
    glm::vec3 dir = glm::normalize(ray.Direction);

    for (auto e : view) {
        auto& t = view.get<Transform>(e);
        auto& c = view.get<AABBCollider>(e);

        glm::vec3 center = t.LocalPosition + c.Offset;
        AABB aabb = AABB::FromCenterHalfSize(center, c.HalfSize);

        float distance;
        glm::vec3 normal;

        if (!IntersectRayAABB(ray, aabb, distance, normal))
            continue;

        if (distance > closest)
            continue;

        closest = distance;

        outHit.HitEntity = Entity(e, &registry);
        outHit.Distance = distance;
        outHit.Point = ray.Origin + dir * distance;
        outHit.Normal = normal;

        hitAnything = true;
    }

    return hitAnything;
}

bool RaycastSphere(Scene& scene, const Ray& ray, RaycastHit& outHit, float maxDistance, entt::entity excludeEntity) {
    auto& registry = scene.Registry();
    auto view = registry.view<Transform, SphereCollider>();

    bool hitAnything = false;
    float closest = maxDistance;
    glm::vec3 dir = glm::normalize(ray.Direction);

    for (auto e : view) {
        if (e == excludeEntity)
            continue;

        auto& t = view.get<Transform>(e);
        auto& c = view.get<SphereCollider>(e);

        Sphere sphere {t.LocalPosition + c.Offset, c.Radius};

        float distance;
        if (!IntersectRaySphere(ray, sphere, distance))
            continue;

        if (distance > closest)
            continue;

        closest = distance;

        outHit.HitEntity = Entity(e, &registry);
        outHit.Distance = distance;
        outHit.Point = ray.Origin + dir * distance;
        outHit.Normal = glm::normalize(outHit.Point - sphere.Center);

        hitAnything = true;
    }

    return hitAnything;
}

// Ray vs finite capsule (Ericson-style cylinder test, clipped to the segment via the axis-projection
// parameter `s`, plus the two sphere caps at the segment endpoints). Reuses IntersectRaySphere for the
// caps rather than duplicating the quadratic. Returns the nearest of the three valid sub-hits.
bool IntersectRayCapsule(const Ray& ray, const Capsule& capsule, float& outT, glm::vec3& outNormal) {
    glm::vec3 dir = glm::normalize(ray.Direction);
    glm::vec3 a = capsule.PointA();
    glm::vec3 b = capsule.PointB();
    glm::vec3 axis = b - a;
    float axisLen = glm::length(axis);

    // Degenerate (near-zero HalfHeight) capsule - just a sphere at its center.
    if (axisLen < 1e-6f)
        return IntersectRaySphere(ray, Sphere {capsule.Center, capsule.Radius}, outT);

    glm::vec3 axisDir = axis / axisLen;
    float bestT = -1.0f;
    glm::vec3 bestNormal {0.0f};

    auto considerCandidate = [&](float t, const glm::vec3& normal) {
        if (t >= 0.0f && (bestT < 0.0f || t < bestT)) {
            bestT = t;
            bestNormal = normal;
        }
    };

    // Infinite-cylinder wall, clamped to the segment's finite extent via `s` (the hit point's
    // projection onto the axis) - a hit outside [0, axisLen] belongs to one of the sphere caps below
    // instead.
    glm::vec3 m = ray.Origin - a;
    glm::vec3 d = dir - axisDir * glm::dot(dir, axisDir);
    glm::vec3 mPerp = m - axisDir * glm::dot(m, axisDir);

    float qa = glm::dot(d, d);
    if (qa > 1e-8f) {
        float qb = 2.0f * glm::dot(mPerp, d);
        float qc = glm::dot(mPerp, mPerp) - capsule.Radius * capsule.Radius;
        float discriminant = qb * qb - 4.0f * qa * qc;
        if (discriminant >= 0.0f) {
            float sqrtDisc = sqrt(discriminant);
            float roots[2] = {(-qb - sqrtDisc) / (2.0f * qa), (-qb + sqrtDisc) / (2.0f * qa)};
            for (float t : roots) {
                if (t < 0.0f)
                    continue;
                glm::vec3 point = ray.Origin + dir * t;
                float s = glm::dot(point - a, axisDir);
                if (s < 0.0f || s > axisLen)
                    continue; // outside the segment - the cap spheres below cover this region
                glm::vec3 axisPoint = a + axisDir * s;
                considerCandidate(t, glm::normalize(point - axisPoint));
            }
        }
    }

    float capT;
    if (IntersectRaySphere(ray, Sphere {a, capsule.Radius}, capT))
        considerCandidate(capT, glm::normalize(ray.Origin + dir * capT - a));
    if (IntersectRaySphere(ray, Sphere {b, capsule.Radius}, capT))
        considerCandidate(capT, glm::normalize(ray.Origin + dir * capT - b));

    if (bestT < 0.0f)
        return false;

    outT = bestT;
    outNormal = bestNormal;
    return true;
}

bool RaycastDynamicColliders(Scene& scene, const Ray& ray, RaycastHit& outHit, float maxDistance,
                             entt::entity excludeEntity) {
    auto& registry = scene.Registry();
    glm::vec3 dir = glm::normalize(ray.Direction);

    bool hitAnything = false;
    float closest = maxDistance;

    auto aabbView = registry.view<Transform, AABBCollider>();
    for (auto e : aabbView) {
        if (e == excludeEntity)
            continue;

        auto& t = aabbView.get<Transform>(e);
        auto& c = aabbView.get<AABBCollider>(e);
        AABB aabb = AABB::FromCenterHalfSize(t.LocalPosition + c.Offset, c.HalfSize);

        float distance;
        glm::vec3 normal;
        if (!IntersectRayAABB(ray, aabb, distance, normal) || distance > closest)
            continue;

        closest = distance;
        outHit.HitEntity = Entity(e, &registry);
        outHit.Distance = distance;
        outHit.Point = ray.Origin + dir * distance;
        outHit.Normal = normal;
        hitAnything = true;
    }

    auto sphereView = registry.view<Transform, SphereCollider>();
    for (auto e : sphereView) {
        if (e == excludeEntity)
            continue;

        auto& t = sphereView.get<Transform>(e);
        auto& c = sphereView.get<SphereCollider>(e);
        Sphere sphere {t.LocalPosition + c.Offset, c.Radius};

        float distance;
        if (!IntersectRaySphere(ray, sphere, distance) || distance > closest)
            continue;

        closest = distance;
        outHit.HitEntity = Entity(e, &registry);
        outHit.Distance = distance;
        outHit.Point = ray.Origin + dir * distance;
        outHit.Normal = glm::normalize(outHit.Point - sphere.Center);
        hitAnything = true;
    }

    auto capsuleView = registry.view<Transform, CapsuleCollider>();
    for (auto e : capsuleView) {
        if (e == excludeEntity)
            continue;

        auto& t = capsuleView.get<Transform>(e);
        auto& c = capsuleView.get<CapsuleCollider>(e);
        Capsule capsule {t.LocalPosition + c.Offset, c.Radius, c.HalfHeight};

        float distance;
        glm::vec3 normal;
        if (!IntersectRayCapsule(ray, capsule, distance, normal) || distance > closest)
            continue;

        closest = distance;
        outHit.HitEntity = Entity(e, &registry);
        outHit.Distance = distance;
        outHit.Point = ray.Origin + dir * distance;
        outHit.Normal = normal;
        hitAnything = true;
    }

    return hitAnything;
}

namespace {

// Standard Moeller-Trumbore ray-triangle test. Nothing like this existed anywhere in the engine
// before RaycastMesh needed it (TriangleMesh's own collision code only does closest-point/SAT
// tests against other shapes, never against a ray).
bool IntersectRayTriangle(const Ray& ray, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, float& outT) {
    constexpr float kEpsilon = 1e-7f;
    glm::vec3 dir = glm::normalize(ray.Direction);

    glm::vec3 edge1 = p1 - p0;
    glm::vec3 edge2 = p2 - p0;
    glm::vec3 pvec = glm::cross(dir, edge2);
    float det = glm::dot(edge1, pvec);
    if (std::abs(det) < kEpsilon)
        return false; // ray parallel to the triangle's plane

    float invDet = 1.0f / det;
    glm::vec3 tvec = ray.Origin - p0;
    float u = glm::dot(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f)
        return false;

    glm::vec3 qvec = glm::cross(tvec, edge1);
    float v = glm::dot(dir, qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f)
        return false;

    float t = glm::dot(edge2, qvec) * invDet;
    if (t < 0.0f)
        return false;

    outT = t;
    return true;
}

} // namespace

bool RaycastMesh(Scene& scene, const Ray& ray, RaycastHit& outHit, float maxDistance) {
    auto& registry = scene.Registry();
    auto view = registry.view<Transform, MeshCollider>();

    bool hitAnything = false;
    float closest = maxDistance;
    glm::vec3 dir = glm::normalize(ray.Direction);

    for (auto e : view) {
        auto& t = view.get<Transform>(e);
        auto& collider = view.get<MeshCollider>(e);

        if (!collider.Mesh)
            continue;

        glm::vec3 origin = t.LocalPosition + collider.Offset;
        AABB worldBounds {collider.Mesh->LocalBounds().Min + origin, collider.Mesh->LocalBounds().Max + origin};

        // Whole-mesh reject before touching any triangle, same reasoning as TriangleMesh's own
        // narrow-phase collision functions (SphereMeshCollision.cpp etc).
        float boundsT;
        glm::vec3 boundsNormal;
        if (!IntersectRayAABB(ray, worldBounds, boundsT, boundsNormal) || boundsT > closest)
            continue;

        size_t triCount = collider.Mesh->GetTriangleCount();
        for (size_t i = 0; i < triCount; i++) {
            glm::vec3 p0, p1, p2;
            collider.Mesh->GetTriangle(i, p0, p1, p2);
            p0 += origin;
            p1 += origin;
            p2 += origin;

            float triT;
            if (!IntersectRayTriangle(ray, p0, p1, p2, triT) || triT > closest)
                continue;

            closest = triT;

            outHit.HitEntity = Entity(e, &registry);
            outHit.Distance = triT;
            outHit.Point = ray.Origin + dir * triT;
            outHit.Normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));

            hitAnything = true;
        }
    }

    return hitAnything;
}

} // namespace Wankel
