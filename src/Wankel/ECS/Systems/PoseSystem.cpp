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

        bool poseChanged = poseSet.Current != poseSet.Previous;
        if (poseChanged) {
            // Re-target from wherever the transform actually is right now, not the old pose's raw
            // value - so changing the target again mid-transition doesn't jump.
            poseSet.Blend.Position = tc.LocalPosition;
            poseSet.Blend.Orientation = tc.LocalOrientation;
            poseSet.Elapsed = 0.0f;
            poseSet.Previous = poseSet.Current;
        }

        const Pose& target = poseSet.Poses[poseSet.Current];

        if (poseChanged) {
            // Swap this pose's sway tuning onto the entity's live MeshAnimation, if it has one -
            // tuning only (see MotionLink::CopyTuning), so the spring keeps moving continuously
            // instead of popping to rest.
            if (auto* meshAnim = registry.try_get<MeshAnimation>(entity)) {
                for (int in = 0; in < MeshAnimation::AxisCount; in++)
                    for (int out = 0; out < MeshAnimation::AxisCount; out++)
                        meshAnim->Links[in][out].CopyTuning(target.Animation.Links[in][out]);
                meshAnim->RotationOrigin = target.Animation.RotationOrigin;
            }
        }

        poseSet.Elapsed += dt;
        float t = target.Duration <= 0.0f ? 1.0f : glm::clamp(poseSet.Elapsed / target.Duration, 0.0f, 1.0f);
        float eased = Ease(target.Ease, t, target.EaseExponent);

        tc.LocalPosition = glm::mix(poseSet.Blend.Position, target.Position, eased);
        tc.LocalOrientation = glm::slerp(poseSet.Blend.Orientation, target.Orientation, eased);
    }
}
} // namespace Wankel
