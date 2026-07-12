#pragma once

#include <vector>
#include <cstdint>

#include "Wankel/Core/Base.h"

namespace Wankel {

// A short in-memory mono PCM (float32) sample. Deliberately basic - just
// enough to back procedurally generated test tones today; loading real
// sound files (via miniaudio's own decoders) is a natural, small follow-on
// once there's an actual asset to load. See Documents/TODO.md.
class AudioClip {
public:
    // Generates a short sine-wave tone with a linear fade-out envelope at
    // the end (avoids an audible click/pop from ending on a nonzero sample).
    static Ref<AudioClip> CreateTone(float frequencyHz, float durationSeconds, float amplitude = 0.4f);

    const std::vector<float>& GetSamples() const { return m_Samples; }
    uint32_t GetSampleRate() const { return m_SampleRate; }

private:
    std::vector<float> m_Samples; // mono
    uint32_t m_SampleRate = 44100;
};

} // namespace Wankel
