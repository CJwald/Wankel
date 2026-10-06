#include "wkpch.h"
#include "PoseSystem.h"

#include "Wankel/ECS/Scene.h"
#include "Wankel/ECS/Components.h"
#include "Wankel/Math/Easing.h"

#include <glm/gtc/quaternion.hpp>


namespace Wankel {

void PoseSystem::Update(Scene& scene, float dt) {
    auto& registry = scene.Registry();
    auto view = registry.view<Transform, PoseSet>();

    for (auto entity : view) {
        auto& tc = view.get<Transform>(entity);
        auto& poseSet = view.get<PoseSet>(entity);
        if (poseSet.Poses.empty())
            continue;
        if (poseSet.Current < 0 || poseSet.Current >= (int)poseSet.Poses.size())
            poseSet.Current = 0; // the pose list was rebuilt shorter

        bool poseChanged = poseSet.Current != poseSet.Previous;
        // An animated target (AnimationPlayerSystem) replaces Poses[Current]; entering, leaving or switching
        // clips re-blends the same way a pose change does.
        bool sourceChanged = poseSet.Animated != poseSet.WasAnimated ||
                             (poseSet.Animated && poseSet.AnimatedSerial != poseSet.LastAnimatedSerial);
        bool retarget = sourceChanged || (poseChanged && !poseSet.Animated);
        if (retarget) {
            // Re-target from wherever the transform actually is right now, not the old pose's raw
            // value - so changing the target again mid-transition doesn't jump.
            poseSet.Blend.Position = tc.LocalPosition;
            poseSet.Blend.Orientation = glm::inverse(poseSet.AdditiveRotation) * tc.LocalOrientation;
            poseSet.Elapsed = 0.0f;
        }
        poseSet.Previous = poseSet.Current;
        poseSet.WasAnimated = poseSet.Animated;
        poseSet.LastAnimatedSerial = poseSet.AnimatedSerial;

        const Pose& target = poseSet.Poses[poseSet.Current];

        if (poseChanged) {
            // Swap this pose's sway tuning onto the entity's live Mesh/TransformAnimation, if it has one -
            // tuning only (see MotionLink::CopyTuning), so the spring keeps moving instead of popping to rest.
            auto applyTuning = [&](ProceduralMotion& live) {
                for (int in = 0; in < ProceduralMotion::AxisCount; in++)
                    for (int out = 0; out < ProceduralMotion::AxisCount; out++)
                        live.Links[in][out].CopyTuning(target.Animation.Links[in][out]);
                live.RotationOrigin = target.Animation.RotationOrigin;
            };
            if (auto* meshAnim = registry.try_get<MeshAnimation>(entity))
                applyTuning(*meshAnim);
            if (auto* transformAnim = registry.try_get<TransformAnimation>(entity))
                applyTuning(*transformAnim);
        }

        poseSet.Elapsed += dt;
        glm::vec3 targetPosition = target.Position;
        glm::quat targetOrientation = target.Orientation;
        float eased;
        if (poseSet.Animated) {
            targetPosition = poseSet.AnimatedPosition;
            targetOrientation = poseSet.AnimatedOrientation;
            eased = poseSet.AnimatedBlendTime <= 0.0f
                        ? 1.0f
                        : Ease(EaseType::SmoothStep, glm::clamp(poseSet.Elapsed / poseSet.AnimatedBlendTime, 0.0f, 1.0f));
        } else {
            float t = target.Duration <= 0.0f ? 1.0f : glm::clamp(poseSet.Elapsed / target.Duration, 0.0f, 1.0f);
            eased = Ease(target.Ease, t, target.EaseExponent);
        }

        tc.LocalPosition = glm::mix(poseSet.Blend.Position, targetPosition, eased);
        tc.LocalOrientation =
            poseSet.AdditiveRotation * glm::slerp(poseSet.Blend.Orientation, targetOrientation, eased);
    }
}
} // namespace Wankel
