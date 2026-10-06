#pragma once

#include "Wankel/ECS/Components/PoseComponents.h"
#include "Wankel/Math/Easing.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace Wankel {

// One pose per animated target, keyed by the target's name in the clip (see AnimationPlayer::Targets).
using PoseMap = std::unordered_map<std::string, Pose>;

// "At Time seconds into the clip, these targets are in these poses."
struct AnimationKeyframe {
    float Time = 0.0f;
    PoseMap Poses;
    EaseType Ease = EaseType::Linear; // shapes the blend from the previous keyframe into this one
};

// Keyframes sorted by Time, which needn't be evenly spaced. A single keyframe is a held pose.
struct AnimationClip {
    std::string Name;
    float Duration = 1.0f;
    bool Looping = true;
    std::vector<AnimationKeyframe> Keyframes;

    void SortKeyframes();
};

// Where a time falls in a clip: blend Alpha of the way from keyframe From to keyframe To. Valid is false for
// an empty clip.
struct ClipSample {
    bool Valid = false;
    int From = 0;
    int To = 0;
    float Alpha = 0.0f;
};

// Looping clips wrap (the last keyframe blends back into the first across the end); others hold their ends.
ClipSample SampleClip(const AnimationClip& clip, float time);

// The target's pose at a sample. A target missing from one of the two keyframes takes the other's pose;
// returns false if neither has it.
bool EvaluateTarget(const AnimationClip& clip, const ClipSample& sample, const std::string& target, Pose& out);

// Every target's pose at `time` - convenience for tools; per-frame playback uses SampleClip/EvaluateTarget.
PoseMap EvaluateClip(const AnimationClip& clip, float time);

} // namespace Wankel
