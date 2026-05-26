#pragma once

#include "VoxelDensityField.h"

#include <vector>

namespace Wankel {

struct MCVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
};

struct MCMeshData {
    std::vector<MCVertex> Vertices;
    std::vector<uint32_t> Indices;
};

class MarchingCubes {
public:

    static MCMeshData Generate(
        const VoxelDensityField& field,
        float isoLevel = 0.0f
    );
};

}
