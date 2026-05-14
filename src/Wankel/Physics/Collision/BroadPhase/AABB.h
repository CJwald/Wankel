#pragma once
#include <glm/glm.hpp>

namespace Wankel {

struct AABB {
    glm::vec3 Min;
    glm::vec3 Max;

    static AABB FromCenterHalfSize(const glm::vec3& center, const glm::vec3& halfSize) {
        return { center - halfSize, center + halfSize };
    }

    bool Intersects(const AABB& other) const {
        return (Min.x <= other.Max.x && Max.x >= other.Min.x) &&
               (Min.y <= other.Max.y && Max.y >= other.Min.y) &&
               (Min.z <= other.Max.z && Max.z >= other.Min.z);
    }
};

}
