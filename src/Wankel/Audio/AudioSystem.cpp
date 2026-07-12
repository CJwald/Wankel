#include "wkpch.h"
#include "AudioSystem.h"
#include "AudioClip.h"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace Wankel {

namespace {

constexpr int kVoiceCount = 8;

struct Voice {
    ma_audio_buffer_ref BufferRef {};
    ma_sound Sound {};
    Ref<AudioClip> ClipRef; // keeps the clip's PCM data alive for as long as
                             // this voice references it, regardless of what
                             // the caller does with its own Ref afterward.
    bool Initialized = false;
};

ma_engine s_Engine;
Voice s_Voices[kVoiceCount];
int s_NextVoice = 0;
bool s_EngineInitialized = false;

void ReleaseVoice(Voice& voice) {
    if (!voice.Initialized)
        return;

    ma_sound_uninit(&voice.Sound);
    ma_audio_buffer_ref_uninit(&voice.BufferRef);
    voice.ClipRef.reset();
    voice.Initialized = false;
}

} // namespace

void AudioSystem::Init() {
    ma_result result = ma_engine_init(nullptr, &s_Engine);
    if (result != MA_SUCCESS) {
        WK_CORE_ERROR("AudioSystem: ma_engine_init failed ({0})", (int)result);
        return;
    }

    s_EngineInitialized = true;
    WK_CORE_INFO("AudioSystem: initialized ({0} voices)", kVoiceCount);
}

void AudioSystem::Shutdown() {
    if (!s_EngineInitialized)
        return;

    for (auto& voice : s_Voices)
        ReleaseVoice(voice);

    ma_engine_uninit(&s_Engine);
    s_EngineInitialized = false;
}

void AudioSystem::Play(const Ref<AudioClip>& clip, float volume) {
    if (!s_EngineInitialized || !clip || clip->GetSamples().empty())
        return;

    Voice& voice = s_Voices[s_NextVoice];
    s_NextVoice = (s_NextVoice + 1) % kVoiceCount;

    ReleaseVoice(voice); // voice stealing - cut short whatever was playing here

    ma_result result = ma_audio_buffer_ref_init(ma_format_f32, 1, clip->GetSamples().data(),
                                                 clip->GetSamples().size(), &voice.BufferRef);
    if (result != MA_SUCCESS) {
        WK_CORE_WARNING("AudioSystem::Play - ma_audio_buffer_ref_init failed ({0})", (int)result);
        return;
    }

    // ma_audio_buffer_ref_init() hardcodes sampleRate to 0 (miniaudio's own
    // source has a "TODO: Version 0.12, set this to sampleRate" comment on
    // that line) - it's a public struct field, and this is the documented
    // workaround.
    voice.BufferRef.sampleRate = clip->GetSampleRate();

    result =
        ma_sound_init_from_data_source(&s_Engine, &voice.BufferRef, MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr, &voice.Sound);
    if (result != MA_SUCCESS) {
        WK_CORE_WARNING("AudioSystem::Play - ma_sound_init_from_data_source failed ({0})", (int)result);
        ma_audio_buffer_ref_uninit(&voice.BufferRef);
        return;
    }

    voice.ClipRef = clip;
    voice.Initialized = true;

    ma_sound_set_volume(&voice.Sound, volume);
    result = ma_sound_start(&voice.Sound);

    WK_CORE_INFO("AudioSystem::Play - voice={0} startResult={1} isPlaying={2} atEnd={3}", s_NextVoice, (int)result,
                 (int)ma_sound_is_playing(&voice.Sound), (int)ma_sound_at_end(&voice.Sound));
}

} // namespace Wankel
