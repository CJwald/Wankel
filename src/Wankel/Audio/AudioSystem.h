#pragma once

#include "Wankel/Core/Base.h"

namespace Wankel {

class AudioClip;

// Minimal fire-and-forget SFX playback: a small fixed pool of voices,
// cycled round-robin with "voice stealing" when all voices are busy - the
// standard, simplest approach for non-spatialized one-shot sound effects.
// No streaming, no 3D spatialization, no music/looping layer; see
// Documents/TODO.md for what a fuller audio system would add.
class AudioSystem {
public:
    static void Init();
    static void Shutdown();

    static void Play(const Ref<AudioClip>& clip, float volume = 1.0f);
};

} // namespace Wankel
