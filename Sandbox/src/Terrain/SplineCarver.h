#pragma once

#include "Wankel/Terrain/VoxelDensityField.h"
#include "Wankel/Math/Spline3D.h"

namespace Wankel {

class SplineCarver {
public:
    static void Carve(VoxelDensityField& field, const Spline3D& spline, float radius, int samples = 128) {
        for (int i = 0; i < samples; i++) {
            float t = (float)i / (samples - 1);

            glm::vec3 center = spline.Sample(t);

            CarveSphere(field, center, radius);
        }
    }

private:
    static void CarveSphere(VoxelDensityField& field, glm::vec3 center, float radius) {
        int r = (int)(radius / field.VoxelSize) + 2;

        glm::ivec3 gc = center / field.VoxelSize;

        for (int z = -r; z <= r; z++) {
            for (int y = -r; y <= r; y++) {
                for (int x = -r; x <= r; x++) {
                    int gx = gc.x + x;
                    int gy = gc.y + y;
                    int gz = gc.z + z;

                    if (!field.InBounds(gx, gy, gz))
                        continue;

                    glm::vec3 wp = field.GridToWorld(gx, gy, gz);

                    float d = glm::distance(wp, center);

                    float sdf = radius - d;

                    field.At(gx, gy, gz) = glm::min(field.At(gx, gy, gz), -sdf);
                }
            }
        }
    }
};

} // namespace Wankel
