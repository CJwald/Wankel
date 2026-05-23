#pragma once

#include <cstdint>
#include <vector>
#include "Wankel/Renderer/Mesh.h"

namespace Geometry {

inline const std::vector<Wankel::Vertex> CubeVertices = {

    // Front
    { {  0.5f,  0.5f,  0.5f }, {0.824f,1.0f,0.0f,1.0f} },
    { { -0.5f,  0.5f,  0.5f }, {0.549f,0.780f,0.039f,1.0f} },
    { { -0.5f, -0.5f,  0.5f }, {0.1f,0.5f,0.0f,1.0f} },
    { {  0.5f, -0.5f,  0.5f }, {0.05f,0.25f,0.0f,1.0f} },

    // Back
    { {  0.5f,  0.5f, -0.5f }, {0.7f,0.9f,0.0f,1.0f} },
    { { -0.5f,  0.5f, -0.5f }, {0.5f,1.0f,0.0f,1.0f} },
    { { -0.5f, -0.5f, -0.5f }, {0.02f,0.3f,0.0f,1.0f} },
    { {  0.5f, -0.5f, -0.5f }, {0.1f,0.0f,0.3f,1.0f} },

};

inline const std::vector<uint32_t> CubeIndices = {
    0,1,2, 0,2,3,
    4,6,5, 4,7,6,
    0,4,5, 0,5,1,
    7,2,6, 7,3,2,
    1,5,6, 1,6,2,
    0,3,7, 0,7,4
};

}
