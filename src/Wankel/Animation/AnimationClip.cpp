#include "wkpch.h"
#include "AnimationClip.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

namespace Wankel {

void AnimationClip::SortKeyframes() {
    std::stable_sort(Keyframes.begin(), Keyframes.end(),
                     [](const AnimationKeyframe& a, const AnimationKeyframe& b) { return a.Time < b.Time; });
}

ClipSample SampleClip(const AnimationClip& clip, float time) {
    ClipSample sample;
    const auto& keys = clip.Keyframes;
    if (keys.empty())
        return sample;
    sample.Valid = true;
    int count = (int)keys.size();
    if (count == 1)
        return sample;

    float duration = std::max(clip.Duration, keys.back().Time);
    if (clip.Looping && duration > 0.0f) {
        time = std::fmod(time, duration);
        if (time < 0.0f)
            time += duration;
    } else {
        time = glm::clamp(time, 0.0f, duration);
    }

    // Before the first or after the last keyframe: hold it, or (looping) blend across the end of the clip.
    if (time < keys.front().Time || time >= keys.back().Time) {
        if (!clip.Looping) {
            sample.From = sample.To = time < keys.front().Time ? 0 : count - 1;
            return sample;
        }
        float gap = keys.front().Time + (duration - keys.back().Time);
        float into = time >= keys.back().Time ? time - keys.back().Time : time + (duration - keys.back().Time);
        sample.From = count - 1;
        sample.To = 0;
        sample.Alpha = gap > 1e-6f ? Ease(keys.front().Ease, glm::clamp(into / gap, 0.0f, 1.0f)) : 0.0f;
        return sample;
    }

    int to = 1;
    while (to < count - 1 && keys[to].Time <= time)
        to++;
    int from = to - 1;
    float span = keys[to].Time - keys[from].Time;
    sample.From = from;
    sample.To = to;
    sample.Alpha = span > 1e-6f ? Ease(keys[to].Ease, glm::clamp((time - keys[from].Time) / span, 0.0f, 1.0f)) : 1.0f;
    return sample;
}

bool EvaluateTarget(const AnimationClip& clip, const ClipSample& sample, const std::string& target, Pose& out) {
    if (!sample.Valid)
        return false;
    const PoseMap& fromPoses = clip.Keyframes[sample.From].Poses;
    const PoseMap& toPoses = clip.Keyframes[sample.To].Poses;
    auto from = fromPoses.find(target);
    auto to = toPoses.find(target);
    if (from == fromPoses.end() && to == toPoses.end())
        return false;
    if (from == fromPoses.end())
        out = to->second;
    else if (to == toPoses.end())
        out = from->second;
    else
        out = LerpPose(from->second, to->second, sample.Alpha);
    return true;
}

PoseMap EvaluateClip(const AnimationClip& clip, float time) {
    PoseMap result;
    ClipSample sample = SampleClip(clip, time);
    if (!sample.Valid)
        return result;
    for (int index : {sample.From, sample.To})
        for (const auto& [target, pose] : clip.Keyframes[index].Poses)
            if (!result.contains(target))
                EvaluateTarget(clip, sample, target, result[target]);
    return result;
}

} // namespace Wankel
