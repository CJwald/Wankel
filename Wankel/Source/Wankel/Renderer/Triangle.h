#pragma once

#include <glm/glm.hpp>
#include <vector>

struct Triangle {
    Triangle(std::vector<glm::vec3> V) {
		std::vector<glm::vec3> Verts = V;
        glm::vec3 e1 = Verts[1] - Verts[0];
        glm::vec3 e2 = Verts[2] - Verts[0];
        glm::vec3 Normal = glm::cross(e1, e2); // note, this assumes points are stored in CCW order
    }
};