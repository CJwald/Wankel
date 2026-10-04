#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

#include "Wankel/ECS/Components/AnimationComponents.h"
#include "Wankel/Math/Easing.h"


namespace Wankel {

struct Pose {
    glm::vec3 Position {0.0f};
    glm::quat Orientation {1, 0, 0, 0};

    // How PoseSystem eases into this pose once it becomes the target.
    EaseType Ease = EaseType::Linear;
    float EaseExponent = 2.0f; // only meaningful for EaseIn/EaseOut
    float Duration = 0.0f;     // seconds to transition INTO this pose; 0 = instant snap

    // This pose's sway tuning - applied to the entity's live Mesh/TransformAnimation on activation.
    ProceduralMotion Animation;
};

struct PoseSet {
    std::vector<Pose> Poses;
    int Current = 0; // index into Poses - the target pose, set by gameplay code
    // Applied on top of the blended pose, in the parent's frame - for procedural adjustments (e.g. a hip yaw
    // follow) that must layer over whichever pose is active instead of fighting PoseSystem for the Transform.
    glm::quat AdditiveRotation {1.0f, 0.0f, 0.0f, 0.0f};

    // Internal - PoseSystem's own bookkeeping, not meant to be set by callers.
    int Previous = -1;
    Pose Blend {}; // snapshot of {Position, Orientation} captured when a transition starts
    float Elapsed = 0.0f;
};

} // namespace Wankel
