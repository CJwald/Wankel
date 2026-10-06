#include "wkpch.h"
#include "AnimationPlayerSystem.h"

#include "Wankel/ECS/Scene.h"
#include "Wankel/ECS/Components.h"

#include <cmath>

namespace Wankel {

void AnimationPlayerSystem::Update(Scene& scene, float dt) {
    auto& registry = scene.Registry();
    auto view = registry.view<AnimationPlayer>();
    for (auto entity : view) {
        auto& player = view.get<AnimationPlayer>(entity);

        if (player.Clip && player.Playing) {
            player.Time += dt * player.Speed;
            float duration = player.Clip->Duration;
            if (!player.Clip->Looping && duration > 0.0f && (player.Time >= duration || player.Time <= 0.0f)) {
                player.Time = glm::clamp(player.Time, 0.0f, duration);
                player.Playing = false; // holds its last pose until stopped or replaced
            } else if (player.Clip->Looping && duration > 0.0f) {
                player.Time = std::fmod(player.Time, duration);
                if (player.Time < 0.0f)
                    player.Time += duration;
            }
        }

        ClipSample sample = player.Clip ? SampleClip(*player.Clip, player.Time) : ClipSample {};
        for (const auto& [name, target] : player.Targets) {
            if (!registry.valid(target))
                continue;
            auto* poseSet = registry.try_get<PoseSet>(target);
            if (!poseSet)
                continue;
            Pose pose;
            poseSet->Animated =
                player.Clip && poseSet->AllowAnimation && EvaluateTarget(*player.Clip, sample, name, pose);
            if (!poseSet->Animated)
                continue;
            poseSet->AnimatedPosition = pose.Position;
            poseSet->AnimatedOrientation = pose.Orientation;
            poseSet->AnimatedBlendTime = player.BlendTime;
            poseSet->AnimatedSerial = player.PlaySerial;
        }
    }
}

} // namespace Wankel
