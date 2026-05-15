#pragma once

#include <cstdint>

namespace Geometry {

inline constexpr float PlateSize = 500.0f;
inline constexpr float PlateHeight = 2.0f; // thickness

inline constexpr float PlateVertices[] = {

    // TOP FACE (y = +h)
     PlateSize,  PlateHeight,  PlateSize,  0.45f, 0.35f, 0.25f, 1.0f,
    -PlateSize,  PlateHeight,  PlateSize,  0.42f, 0.33f, 0.23f, 1.0f,
    -PlateSize,  PlateHeight, -PlateSize,  0.40f, 0.32f, 0.22f, 1.0f,
     PlateSize,  PlateHeight, -PlateSize,  0.44f, 0.34f, 0.24f, 1.0f,

    // BOTTOM FACE (y = -h)
     PlateSize, -PlateHeight,  PlateSize,  0.25f, 0.20f, 0.15f, 1.0f,
    -PlateSize, -PlateHeight,  PlateSize,  0.22f, 0.18f, 0.12f, 1.0f,
    -PlateSize, -PlateHeight, -PlateSize,  0.20f, 0.16f, 0.10f, 1.0f,
     PlateSize, -PlateHeight, -PlateSize,  0.23f, 0.19f, 0.13f, 1.0f,
};

inline constexpr uint32_t PlateIndices[] = {

    // Top
    0, 1, 2,
    0, 2, 3,

    // Bottom
    4, 6, 5,
    4, 7, 6,

    // Sides
    0, 4, 5, 0, 5, 1,
    1, 5, 6, 1, 6, 2,
    2, 6, 7, 2, 7, 3,
    3, 7, 4, 3, 4, 0
};

}
