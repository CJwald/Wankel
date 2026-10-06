#include "wkpch.h"
#include "LocomotionSystem.h"

#include "Wankel/ECS/Scene.h"
#include "Wankel/ECS/Components.h"

namespace Wankel {

void LocomotionSystem::Update(Scene& scene) {
    auto view = scene.Registry().view<Locomotion, AnimationPlayer, Rigidbody>();
    for (auto entity : view) {
        auto& locomotion = view.get<Locomotion>(entity);
        auto& player = view.get<AnimationPlayer>(entity);
        bool onLocomotionClip = player.Clip && (player.Clip == locomotion.IdleClip || player.Clip == locomotion.MovingClip);

        if (!locomotion.Enabled) {
            if (onLocomotionClip)
                player.Stop();
            locomotion.State = LocomotionState::Idle;
            continue;
        }

        glm::vec3 velocity = view.get<Rigidbody>(entity).Velocity;
        if (locomotion.HorizontalOnly)
            velocity.y = 0.0f;
        locomotion.Speed = glm::length(velocity);
        locomotion.State = locomotion.Speed >= locomotion.MoveThreshold ? LocomotionState::Moving : LocomotionState::Idle;

        bool moving = locomotion.State == LocomotionState::Moving;
        const Ref<AnimationClip>& clip = moving ? locomotion.MovingClip : locomotion.IdleClip;
        float speed = 1.0f;
        if (moving) {
            speed = locomotion.MovingPlaybackSpeed;
            if (locomotion.ReferenceSpeed > 0.0f)
                speed *= locomotion.Speed / locomotion.ReferenceSpeed;
        }
        if (!clip) {
            if (onLocomotionClip)
                player.Stop();
            continue;
        }
        // Another clip the game started (not one of ours) is left alone.
        if (player.Clip != clip && (onLocomotionClip || !player.Clip))
            player.Play(clip, speed);
        else if (player.Clip == clip)
            player.Speed = speed;
    }
}

} // namespace Wankel
