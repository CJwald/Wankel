#include "wkpch.h"

#include "Raycast.h"

#include <Wankel/ECS/Scene.h>
#include <Wankel/ECS/Components.h>

#include "../Collision/BroadPhase/AABB.h"
#include "../Collision/NarrowPhase/Sphere.h"

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
    if (std::abs(dir.x) < kEpsilon) dir.x = std::copysign(kEpsilon, dir.x);
    if (std::abs(dir.y) < kEpsilon) dir.y = std::copysign(kEpsilon, dir.y);
    if (std::abs(dir.z) < kEpsilon) dir.z = std::copysign(kEpsilon, dir.z);

    glm::vec3 invDir = 1.0f / dir;

    glm::vec3 t0 = (aabb.Min - ray.Origin) * invDir;
    glm::vec3 t1 = (aabb.Max - ray.Origin) * invDir;

    glm::vec3 tmin = glm::min(t0, t1);
    glm::vec3 tmax = glm::max(t0, t1);

    float nearT = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
    float farT  = glm::min(glm::min(tmax.x, tmax.y), tmax.z);

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

}
