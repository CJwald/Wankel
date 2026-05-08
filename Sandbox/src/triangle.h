#pragma once

#include <cstdint>

namespace Geometry {

inline constexpr float TriangleVertices[] = {
    // positions only (4 unique Triangle corners)
//   X      Y            Z         R     G     B     A
	 0.0f,  0.0f,        1.0f,     0.0f, 0.0f, 1.0f, 0.5f, // 0
	 1.0f,  0.0f,       -0.5f,     1.0f, 0.0f, 0.5f, 0.5f, // 1
	-0.5f,  0.8660254f, -0.5f,     0.0f, 0.9f, 0.0f, 0.5f, // 2
	-0.5f, -0.8660254f, -0.5f,     0.5f, 0.9f, 0.5f, 0.5f, // 3
};

inline constexpr uint32_t TriangleIndices[] = {
    0, 1, 2,
    0, 3, 1,
    0, 2, 3,
    1, 3, 2,
};

}
