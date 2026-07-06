#pragma once

#include <vector>
#include <glm/glm.hpp>

#include <Wankel/Core/Log.h>

namespace Wankel {

class Spline3D {
public:

    std::vector<glm::vec3> Points;

    glm::vec3 Sample(float t) const {

        if (Points.size() < 4) {
            WK_CORE_WARNING("Spline3D::Sample called with fewer than 4 control points ({0}) - returning origin", Points.size());
            return glm::vec3(0);
        }

        int count = (int)Points.size() - 3;

        float scaled = t * count;

        int seg = glm::clamp((int)scaled, 0, count - 1);

        float localT = scaled - seg;

        const glm::vec3& p0 = Points[seg + 0];
        const glm::vec3& p1 = Points[seg + 1];
        const glm::vec3& p2 = Points[seg + 2];
        const glm::vec3& p3 = Points[seg + 3];

        float tt = localT * localT;
        float ttt = tt * localT;

        return 0.5f * (
            (2.0f * p1) +
            (-p0 + p2) * localT +
            (2.0f*p0 - 5.0f*p1 + 4.0f*p2 - p3) * tt +
            (-p0 + 3.0f*p1 - 3.0f*p2 + p3) * ttt
        );
    };
};

}
