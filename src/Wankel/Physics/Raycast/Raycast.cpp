#include "wkpch.h"

#include "Raycast.h"

#include <Wankel/ECS/Scene.h>
#include <Wankel/ECS/Components.h>

#include "../Collision/BroadPhase/AABB.h"
#include "../Collision/NarrowPhase/Sphere.h"

namespace Wankel {

bool IntersectRaySphere(const Ray& ray, const Sphere& sphere, float& outDistance) {
    glm::vec3 oc = ray.Origin - sphere.Center;

    float a = glm::dot(ray.Direction, ray.Direction);
    float b = 2.0f * glm::dot(oc, ray.Direction);
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


bool IntersectRayAABB(const Ray& ray, const AABB& aabb, float& t) {
    glm::vec3 invDir = 1.0f / ray.Direction; // TODO: This can devide by zero. Should add epsilon handling 

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

    return true;
}

bool RaycastAABB(Scene& scene, const Ray& ray, RaycastHit& outHit, float maxDistance) {
    auto& registry = scene.Registry();
    auto view = registry.view<TransformComponent, AABBComponent>();

    bool hitAnything = false;
    float closest = maxDistance;

    for (auto e : view) {
        auto& t = view.get<TransformComponent>(e);
        auto& c = view.get<AABBComponent>(e);

		glm::vec3 center = t.LocalPosition + c.Offset;
        AABB aabb = AABB::FromCenterHalfSize(center, c.HalfSize);

        float distance;

        if (!IntersectRayAABB(ray, aabb, distance))
            continue;

        if (distance > closest)
            continue;

        closest = distance;

        outHit.HitEntity = Entity(e, &registry);
        outHit.Distance = distance;
        outHit.Point = ray.Origin + ray.Direction * distance;

        hitAnything = true;
    }

    return hitAnything;
}

}
