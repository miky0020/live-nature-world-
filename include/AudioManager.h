// ============================================================================
//  AudioManager.h - environmental audio with distance attenuation.
//
//  Section 19 asks for layered ambience that reacts to the weather, the time
//  of day and the player's distance from the campfire, without a game engine's
//  audio system. The honest constraints on a GLUT/MinGW project:
//
//    * PlaySound (winmm) ships with Windows but plays ONE sound at a time with
//      no volume control - useless for a layered mix
//    * miniaudio is a single public-domain header with no extra linking, and
//      gives looping, per-sound volume and simultaneous channels
//
//  So the mixing logic lives here and is always compiled, while the backend is
//  swappable. With no backend the whole thing degrades to a no-op that logs
//  once and never crashes - satisfying section 33.
//
//  TO ENABLE SOUND:
//    1. drop miniaudio.h into include/third_party/
//    2. add LIVINGISLAND_USE_MINIAUDIO to the compiler defines
//    3. put .wav / .mp3 files in assets/audio/ (see the README for names)
// ============================================================================
#ifndef LIVINGISLAND_AUDIOMANAGER_H
#define LIVINGISLAND_AUDIOMANAGER_H

#include "Utilities.h"

#include <string>
#include <vector>

// The logical channels in the mix.
enum AudioChannel {
    AUDIO_AMBIENT_DAY = 0,   // birds, insects, general daytime nature
    AUDIO_AMBIENT_NIGHT,     // crickets, owls
    AUDIO_RAIN,
    AUDIO_WIND,
    AUDIO_CAMPFIRE,          // positional
    AUDIO_MUSIC,
    AUDIO_CHANNEL_COUNT
};

// Everything the mixer needs in order to decide the levels.
struct AudioEnvironment {
    float daylight;           // 0..1
    float rainIntensity;      // 0..1
    float windStrength;       // 0..1
    float campfireDistance;   // world units from the listener
    float campfireGlow;       // 0 when the fire is out
    bool  muted;

    AudioEnvironment()
        : daylight(1.0f), rainIntensity(0.0f), windStrength(0.0f)
        , campfireDistance(999.0f), campfireGlow(1.0f), muted(false) {}
};

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    bool initialize(const std::string& assetDirectory);
    void shutdown();

    void update(float deltaTime, const AudioEnvironment& environment);

    void setMasterVolume(float volume);
    float masterVolume() const { return m_masterVolume; }

    void setMuted(bool muted);
    bool muted() const { return m_muted; }
    void toggleMute() { setMuted(!m_muted); }

    bool available() const {
        if (!m_backendReady) return false;
        for (int i = 0; i < AUDIO_CHANNEL_COUNT; ++i) if (m_loaded[i]) return true;
        return false;
    }

    // Exposed so the debug overlay can show the live mix.
    float channelVolume(AudioChannel channel) const;
    static const char* channelName(AudioChannel channel);

private:
    // Backend hooks. Each has a real implementation when a backend is compiled
    // in, and an empty one otherwise.
    bool backendInitialize();
    void backendShutdown();
    bool backendLoadLooping(AudioChannel channel, const std::string& path);
    void backendSetVolume(AudioChannel channel, float volume);

    // Distance attenuation for the positional sources. Inverse-square would
    // drop off far too abruptly at these scales, so this is a smoothed linear
    // rolloff between a full-volume radius and a silence radius.
    static float attenuation(float distance, float fullVolumeRadius, float silenceRadius);

    std::string m_assetDirectory;
    float m_targetVolume[AUDIO_CHANNEL_COUNT];
    float m_currentVolume[AUDIO_CHANNEL_COUNT];
    bool  m_loaded[AUDIO_CHANNEL_COUNT];

    float m_masterVolume;
    bool  m_muted;
    bool  m_backendReady;
    bool  m_warned;
};

#endif // LIVINGISLAND_AUDIOMANAGER_H
