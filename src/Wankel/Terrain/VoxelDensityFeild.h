#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace Wankel {

class VoxelDensityField {
public:

    int Width;
    int Height;
    int Depth;

    float VoxelSize = 1.0f;

    std::vector<float> Density;

    VoxelDensityField(
        int width,
        int height,
        int depth,
        float voxelSize = 1.0f
    )
        : Width(width),
          Height(height),
          Depth(depth),
          VoxelSize(voxelSize)
    {
        Density.resize(width * height * depth, -1.0f);
    }

    inline int Index(int x, int y, int z) const {
        return x + y * Width + z * Width * Height;
    }

    inline bool InBounds(int x, int y, int z) const {
        return
            x >= 0 && y >= 0 && z >= 0 &&
            x < Width &&
            y < Height &&
            z < Depth;
    }

    inline float& At(int x, int y, int z) {
        return Density[Index(x,y,z)];
    }

    inline const float& At(int x, int y, int z) const {
        return Density[Index(x,y,z)];
    }

    glm::vec3 GridToWorld(int x, int y, int z) const {
        return glm::vec3(
            x * VoxelSize,
            y * VoxelSize,
            z * VoxelSize
        );
    }
};

}
