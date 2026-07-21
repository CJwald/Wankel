#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Wankel/Math/SecondOrderDynamics.h"
#include "Wankel/ECS/Components/MotionProfile.h"


namespace Wankel {

struct MeshAnimation {
    static constexpr int AxisCount = (int)MotionAxis::Count;

    MotionLink Links[AxisCount][AxisCount];

    glm::vec3 PositionOffset {0.0f};
    glm::vec3 RotationOffset {0.0f};

    bool Initialized = false;

    // Enables Links[from][to] and sets its tuning in one call instead of six
    // separate field assignments. Doesn't touch Spring - ProceduralAnimationSystem
    // reconstructs it from Frequency/Damping/Response every frame regardless.
    MotionLink& SetLink(MotionAxis from, MotionAxis to, float magnitude, float frequency, float damping,
                        float response, float clamp) {
        auto& link = Links[(int)from][(int)to];
        link.Enabled = true;
        link.Magnitude = magnitude;
        link.Frequency = frequency;
        link.Damping = damping;
        link.Response = response;
        link.Clamp = clamp;
        return link;
    }
};

} // namespace Wankel
