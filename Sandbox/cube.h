#pragma once

#include <cstdint>

namespace Geometry {

inline constexpr float CubeVertices[] = {
    // positions only (8 unique cube corners)

    // back face
    -0.5f, -0.5f, -0.5f, // 0
     0.5f, -0.5f, -0.5f, // 1
     0.5f,  0.5f, -0.5f, // 2
    -0.5f,  0.5f, -0.5f, // 3

    // front face
    -0.5f, -0.5f,  0.5f, // 4
     0.5f, -0.5f,  0.5f, // 5
     0.5f,  0.5f,  0.5f, // 6
    -0.5f,  0.5f,  0.5f  // 7
};

inline constexpr uint32_t CubeIndices[] = {
    // back face
    0, 1, 2, 2, 3, 0,

    // front face
    4, 5, 6, 6, 7, 4,

    // left face
    4, 0, 3, 3, 7, 4,

    // right face
    1, 5, 6, 6, 2, 1,

    // bottom face
    4, 5, 1, 1, 0, 4,

    // top face
    3, 2, 6, 6, 7, 3
};

}
