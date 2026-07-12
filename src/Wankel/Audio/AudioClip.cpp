#include "wkpch.h"
#include "AudioClip.h"

#include <cmath>
#include <algorithm>

namespace Wankel {

Ref<AudioClip> AudioClip::CreateTone(float frequencyHz, float durationSeconds, float amplitude) {
    auto clip = CreateRef<AudioClip>();

    uint32_t sampleRate = clip->m_SampleRate;
    uint32_t frameCount = (uint32_t)(durationSeconds * (float)sampleRate);

    clip->m_Samples.resize(frameCount);

    constexpr float kTwoPi = 6.28318530718f;
    uint32_t fadeFrames = std::min(frameCount, (uint32_t)(0.01f * (float)sampleRate)); // ~10ms fade-out

    for (uint32_t i = 0; i < frameCount; i++) {
        float t = (float)i / (float)sampleRate;
        float sample = amplitude * std::sin(kTwoPi * frequencyHz * t);

        if (fadeFrames > 0 && i >= frameCount - fadeFrames)
            sample *= (float)(frameCount - i) / (float)fadeFrames;

        clip->m_Samples[i] = sample;
    }

    return clip;
}

} // namespace Wankel
