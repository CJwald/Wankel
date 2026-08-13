#pragma once

#include <cstdint>
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

    // Opaque per-voxel byte, meaning entirely caller-defined - the engine never reads/writes it
    // itself (same "stays gameplay-agnostic" boundary as RaycastHit). Lets a client like Mechtrix
    // track a per-voxel material id without the engine depending on a game-specific type.
    std::vector<uint8_t> MaterialId;

    VoxelDensityField(int width, int height, int depth, float voxelSize = 1.0f)
        : Width(width), Height(height), Depth(depth), VoxelSize(voxelSize) {
        Density.resize(width * height * depth, -1.0f);
        MaterialId.resize(width * height * depth, 0);
    }

    inline int Index(int x, int y, int z) const { return x + y * Width + z * Width * Height; }

    inline bool InBounds(int x, int y, int z) const {
        return x >= 0 && y >= 0 && z >= 0 && x < Width && y < Height && z < Depth;
    }

    inline float& At(int x, int y, int z) { return Density[Index(x, y, z)]; }

    inline const float& At(int x, int y, int z) const { return Density[Index(x, y, z)]; }

    inline uint8_t& MaterialAt(int x, int y, int z) { return MaterialId[Index(x, y, z)]; }

    inline const uint8_t& MaterialAt(int x, int y, int z) const { return MaterialId[Index(x, y, z)]; }

    glm::vec3 GridToWorld(int x, int y, int z) const { return glm::vec3(x * VoxelSize, y * VoxelSize, z * VoxelSize); }
};

} // namespace Wankel
