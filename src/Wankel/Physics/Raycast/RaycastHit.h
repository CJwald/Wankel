#pragma once

#include <glm/glm.hpp>
#include <Wankel/ECS/Entity.h>

namespace Wankel {

struct RaycastHit {
    Entity HitEntity;

    glm::vec3 Point{0.0f};
    glm::vec3 Normal{0.0f};

    float Distance = 0.0f;
};

}
