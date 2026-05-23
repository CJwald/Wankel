#pragma once

#include "Ray.h"
#include "RaycastHit.h"

namespace Wankel {

class Scene;
struct Sphere;

bool IntersectRaySphere(const Ray& ray, const Sphere& sphere, float& outDistance);

bool RaycastAABB(
    Scene& scene,
    const Ray& ray,
    RaycastHit& outHit,
    float maxDistance = 1000.0f
);

}
