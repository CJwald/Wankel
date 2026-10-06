#pragma once

#include "Wankel/Animation/AnimationClip.h"
#include "Wankel/Core/Base.h"

#include <cstdint>

namespace Wankel {

enum class LocomotionState : std::uint8_t { Idle, Moving };

// Picks an entity's AnimationPlayer clip from how fast its Rigidbody is moving (see LocomotionSystem).
// The game supplies the clips and decides when locomotion applies at all (Enabled).
struct Locomotion {
    bool Enabled = true;        // false stops the locomotion clip, handing targets back to their static poses
    float MoveThreshold = 0.3f; // speed (units/s) at or above which the entity counts as Moving
    bool HorizontalOnly = true; // ignore vertical velocity (falling isn't walking)
    Ref<AnimationClip> IdleClip;
    Ref<AnimationClip> MovingClip;
    float MovingPlaybackSpeed = 1.0f;
    float ReferenceSpeed = 0.0f; // > 0: Moving playback scales by speed / ReferenceSpeed on top of the above

    // Runtime - written by LocomotionSystem.
    LocomotionState State = LocomotionState::Idle;
    float Speed = 0.0f;
};

} // namespace Wankel
