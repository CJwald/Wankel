#pragma once

#include "Wankel/Math/Spline3D.h"

#include <random>

namespace Wankel {

class SplineGenerator {
public:

    static Spline3D Generate(
        glm::vec3 start,
        glm::vec3 end,
        int middlePoints,
        float randomness
    )
    {
        Spline3D spline;

        std::mt19937 rng(std::random_device{}());

        std::uniform_real_distribution<float> dist(-randomness, randomness);

        spline.Points.push_back(start);
        spline.Points.push_back(start);

        for (int i = 0; i < middlePoints; i++) {

            float t = (float)(i + 1) / (middlePoints + 1);

            glm::vec3 p = glm::mix(start, end, t);

            p += glm::vec3(
                dist(rng),
                dist(rng),
                dist(rng)
            );

            spline.Points.push_back(p);
        }

        spline.Points.push_back(end);
        spline.Points.push_back(end);

        return spline;
    }
};

}
