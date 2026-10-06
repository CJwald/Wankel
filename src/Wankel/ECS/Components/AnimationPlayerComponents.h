#pragma once

#include "Wankel/Animation/AnimationClip.h"
#include "Wankel/Core/Base.h"

#include <entt/entt.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Wankel {

// Plays one AnimationClip over a set of target entities. AnimationPlayerSystem advances Time and hands each
// target's evaluated pose to its PoseSet, so PoseSystem/the hierarchy apply it like any other pose.
struct AnimationPlayer {
    Ref<AnimationClip> Clip;
    float Time = 0.0f;
    float Speed = 1.0f;
    bool Playing = false;
    float BlendTime = 0.15f; // seconds targets ease into a newly started clip

    // Clip target name -> the entity it drives (each needs a Transform + PoseSet).
    std::vector<std::pair<std::string, entt::entity>> Targets;

    std::uint32_t PlaySerial = 0; // bumped by Play, so targets re-blend into the new clip

    void Play(const Ref<AnimationClip>& clip, float speed = 1.0f, bool restart = true) {
        if (clip != Clip || restart) {
            Time = 0.0f;
            PlaySerial++;
        }
        Clip = clip;
        Speed = speed;
        Playing = clip != nullptr;
    }
    void Stop() {
        Clip = nullptr;
        Playing = false;
        Time = 0.0f;
    }
    void Pause() { Playing = false; }
    void Resume() { Playing = Clip != nullptr; }
    float Duration() const { return Clip ? Clip->Duration : 0.0f; }
};

} // namespace Wankel
