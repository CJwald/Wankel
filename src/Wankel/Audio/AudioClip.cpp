#include "wkpch.h"
#include "AudioClip.h"

#include <miniaudio.h>

#include <cmath>
#include <algorithm>

namespace Wankel {

Ref<AudioClip> AudioClip::CreateTone(float frequencyHz, float durationSeconds, float amplitude) {
    auto clip = CreateRef<AudioClip>();

    // Casting a negative or NaN duration straight to uint32_t below is
    // undefined behavior and can wrap to a huge frame count, turning the
    // resize() a few lines down into a multi-gigabyte allocation attempt.
    // `!(durationSeconds > 0.0f)` also catches NaN, since any comparison
    // against NaN is false. AudioSystem::Play() already no-ops on an empty
    // clip, so returning one here degrades gracefully instead of crashing.
    if (!(durationSeconds > 0.0f)) {
        WK_CORE_WARNING("AudioClip::CreateTone: invalid duration {0}, returning an empty clip", durationSeconds);
        return clip;
    }

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

Ref<AudioClip> AudioClip::LoadFromFile(const std::string& path) {
    auto clip = CreateRef<AudioClip>();

    // outputSampleRate = 0 means "keep the source's native rate" - ma_decode_file writes the actual
    // decoded rate back into config.sampleRate (see ma_decoder__full_decode_and_uninit), which we read
    // below rather than assuming 44100 like CreateTone does.
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 1, 0);
    ma_uint64 frameCount = 0;
    void* pFrames = nullptr;

    ma_result result = ma_decode_file(path.c_str(), &config, &frameCount, &pFrames);
    if (result != MA_SUCCESS) {
        WK_CORE_WARNING("AudioClip::LoadFromFile: failed to decode '{0}' ({1}), returning an empty clip",
                        path, (int)result);
        return clip;
    }

    clip->m_SampleRate = config.sampleRate;
    clip->m_Samples.assign((float*)pFrames, (float*)pFrames + frameCount);
    ma_free(pFrames, nullptr);

    return clip;
}

} // namespace Wankel
