#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdint>
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

// Transform blend between two poses (t = 0 -> a, 1 -> b); the rest of the result is a's.
inline Pose LerpPose(const Pose& a, const Pose& b, float t) {
    Pose out = a;
    out.Position = glm::mix(a.Position, b.Position, t);
    out.Orientation = glm::slerp(a.Orientation, b.Orientation, t);
    return out;
}

struct PoseSet {
    std::vector<Pose> Poses;
    int Current = 0; // index into Poses - the target pose, set by gameplay code
    // Applied on top of the blended pose, in the parent's frame - for procedural adjustments (e.g. a hip yaw
    // follow) that must layer over whichever pose is active instead of fighting PoseSystem for the Transform.
    glm::quat AdditiveRotation {1.0f, 0.0f, 0.0f, 0.0f};
    // False keeps this entity on Poses[Current] even while an AnimationPlayer has a track for it.
    bool AllowAnimation = true;

    // Written each frame by AnimationPlayerSystem while a playing clip drives this entity.
    bool Animated = false;
    glm::vec3 AnimatedPosition {0.0f};
    glm::quat AnimatedOrientation {1.0f, 0.0f, 0.0f, 0.0f};
    float AnimatedBlendTime = 0.0f;  // seconds to ease into the clip when it starts
    std::uint32_t AnimatedSerial = 0; // changes when a new clip starts, so PoseSystem re-blends

    // Internal - PoseSystem's own bookkeeping, not meant to be set by callers.
    int Previous = -1;
    Pose Blend {}; // snapshot of {Position, Orientation} captured when a transition starts
    float Elapsed = 0.0f;
    bool WasAnimated = false;
    std::uint32_t LastAnimatedSerial = 0;
};

} // namespace Wankel
