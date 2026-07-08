#pragma once

#include "Wankel/Terrain/VoxelDensityField.h"

#include <glm/glm.hpp>
#include <random>

namespace Wankel {

class AsteroidGenerator {
public:
    static void Generate(VoxelDensityField& field, glm::vec3 center, float radius, float noiseScale,
                         float surfaceNoise) {
        std::mt19937 rng(std::random_device {}());

        for (int z = 0; z < field.Depth; z++) {
            for (int y = 0; y < field.Height; y++) {
                for (int x = 0; x < field.Width; x++) {
                    glm::vec3 p = field.GridToWorld(x, y, z);

                    glm::vec3 local = p - center;

                    float d = glm::length(local);

                    float noise = sin(local.x * noiseScale) * sin(local.y * noiseScale) * sin(local.z * noiseScale);

                    float surface = radius + noise * surfaceNoise;

                    field.At(x, y, z) = surface - d;
                }
            }
        }
    }
};

} // namespace Wankel
