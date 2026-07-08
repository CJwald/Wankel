#pragma once

#include <glm/glm.hpp>

namespace Wankel {

struct Ray {
    glm::vec3 Origin {0.0f};
    glm::vec3 Direction {0.0f, 0.0f, -1.0f};
};

} // namespace Wankel
