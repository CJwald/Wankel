#pragma once

#include <vector>
#include <cstdint>
#include <string>

#include "Wankel/Core/Base.h"

namespace Wankel {

// A short in-memory mono PCM (float32) sample, either generated procedurally (CreateTone) or decoded
// from a file (LoadFromFile). See Documents/TODO.md.
class AudioClip {
public:
    // Generates a short sine-wave tone with a linear fade-out envelope at
    // the end (avoids an audible click/pop from ending on a nonzero sample).
    static Ref<AudioClip> CreateTone(float frequencyHz, float durationSeconds, float amplitude = 0.4f);

    // Decodes an entire audio file (wav/flac/mp3 - whatever miniaudio's compiled-in decoders support)
    // into an in-memory mono float32 PCM buffer, same shape CreateTone produces. Returns an empty clip
    // (AudioSystem::Play safely no-ops on one) and logs a warning on failure, rather than throwing -
    // audio is meant to degrade gracefully, matching CreateTone's own invalid-input handling above.
    static Ref<AudioClip> LoadFromFile(const std::string& path);

    const std::vector<float>& GetSamples() const { return m_Samples; }
    uint32_t GetSampleRate() const { return m_SampleRate; }

private:
    std::vector<float> m_Samples; // mono
    uint32_t m_SampleRate = 44100;
};

} // namespace Wankel
