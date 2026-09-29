#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Wankel/Math/SecondOrderDynamics.h"
#include "Wankel/ECS/Components/MotionProfile.h"


namespace Wankel {

// Velocity-driven spring sway (Links[in][out]: local motion axis -> offset axis). Shared by MeshAnimation
// and TransformAnimation, which differ only in where the resulting offset is applied.
struct ProceduralMotion {
    static constexpr int AxisCount = (int)MotionAxis::Count;

    MotionLink Links[AxisCount][AxisCount];

    glm::vec3 PositionOffset {0.0f};
    glm::vec3 RotationOffset {0.0f};
    glm::vec3 RotationOrigin {
        0.0f}; // local-space pivot point for RotationOffset; {0,0,0} = rotate around the entity origin

    bool Initialized = false;

    // Enables Links[from][to] and sets its tuning in one call instead of six
    // separate field assignments. Doesn't touch Spring - ProceduralAnimationSystem
    // reconstructs it from Frequency/Damping/Response every frame regardless.
    MotionLink& SetLink(MotionAxis from, MotionAxis to, float magnitude, float frequency, float damping, float response,
                        float clampMin, float clampMax) {
        auto& link = Links[(int)from][(int)to];
        link.Enabled = true;
        link.Magnitude = magnitude;
        link.Frequency = frequency;
        link.Damping = damping;
        link.Response = response;
        link.ClampMin = clampMin;
        link.ClampMax = clampMax;
        return link;
    }
};

// Offset applied to the rendered mesh only (Transform::VisualPosition/VisualRotation -> FinalTransform);
// children don't inherit it. Driven by ProceduralAnimationSystem from the entity's Kinematics.
struct MeshAnimation : ProceduralMotion {};

// Offset applied to the entity's local transform (Transform::AnimationPosition/AnimationRotation ->
// LocalTransform), so children follow it through the hierarchy - e.g. a swinging upper leg carries its
// lower leg and foot. Driven by TransformAnimationSystem.
struct TransformAnimation : ProceduralMotion {
    // Runtime only - the rest frame's (parent world x unanimated local) pose last frame, for its velocity.
    glm::vec3 PreviousRestPosition {0.0f};
    glm::quat PreviousRestOrientation {1, 0, 0, 0};
    bool HasPreviousRest = false;
};

} // namespace Wankel
