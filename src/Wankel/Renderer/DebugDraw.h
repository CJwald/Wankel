#pragma once

#include <glm/glm.hpp>

namespace Wankel {

struct DebugLine {
    glm::vec3 P0;
    glm::vec3 P1;
    glm::vec3 Color;
};

struct DebugTriangle {
    glm::vec3 P0;
    glm::vec3 P1;
    glm::vec3 P2;
    glm::vec3 Color;
};

} // namespace Wankel
