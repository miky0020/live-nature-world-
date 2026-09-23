#include "AudioManager.h"
#include "Config.h"

#include <cstdio>
#include <fstream>

// ---------------------------------------------------------------------------
//  Optional backend
// ---------------------------------------------------------------------------
#ifdef LIVINGISLAND_USE_MINIAUDIO
    #define MINIAUDIO_IMPLEMENTATION
    #include "third_party/miniaudio.h"

    namespace {
        ma_engine g_engine;
        ma_sound  g_sounds[AUDIO_CHANNEL_COUNT];
        bool      g_soundValid[AUDIO_CHANNEL_COUNT] = { false };
        bool      g_engineReady = false;
    }
#endif

// ---------------------------------------------------------------------------
AudioManager::AudioManager()
    : m_masterVolume(0.8f)
    , m_muted(false)
    , m_backendReady(false)
    , m_warned(false)
{
    for (int i = 0; i < AUDIO_CHANNEL_COUNT; ++i) {
        m_targetVolume[i]  = 0.0f;
        m_currentVolume[i] = 0.0f;
        m_loaded[i] = false;
    }
}

AudioManager::~AudioManager() { shutdown(); }

const char* AudioManager::channelName(AudioChannel channel) {
    switch (channel) {
        case AUDIO_AMBIENT_DAY:   return "ambient_day";
        case AUDIO_AMBIENT_NIGHT: return "ambient_night";
        case AUDIO_RAIN:          return "rain";
        case AUDIO_WIND:          return "wind";
        case AUDIO_CAMPFIRE:      return "campfire";
        case AUDIO_MUSIC:         return "music";
        default:                  return "unknown";
    }
}

// ---------------------------------------------------------------------------
//  Backend hooks
// ---------------------------------------------------------------------------
bool AudioManager::backendInitialize() {
#ifdef LIVINGISLAND_USE_MINIAUDIO
    if (ma_engine_init(NULL, &g_engine) != MA_SUCCESS) {
        util::logWarning("Audio engine failed to start - continuing without audio.");
        return false;
    }
    g_engineReady = true;
    return true;
#else
    return false;
#endif
}

void AudioManager::backendShutdown() {
#ifdef LIVINGISLAND_USE_MINIAUDIO
    if (!g_engineReady) return;
    for (int i = 0; i < AUDIO_CHANNEL_COUNT; ++i) {
        if (g_soundValid[i]) {
            ma_sound_stop(&g_sounds[i]);
            ma_sound_uninit(&g_sounds[i]);
            g_soundValid[i] = false;
        }
    }
    ma_engine_uninit(&g_engine);
    g_engineReady = false;
#endif
}

bool AudioManager::backendLoadLooping(AudioChannel channel, const std::string& path) {
#ifdef LIVINGISLAND_USE_MINIAUDIO
    if (!g_engineReady) return false;

    ma_result result = ma_sound_init_from_file(
        &g_engine, path.c_str(), MA_SOUND_FLAG_STREAM, NULL, NULL, &g_sounds[channel]);

    if (result != MA_SUCCESS) return false;

    ma_sound_set_looping(&g_sounds[channel], MA_TRUE);
    ma_sound_set_volume(&g_sounds[channel], 0.0f);
    ma_sound_start(&g_sounds[channel]);
    g_soundValid[channel] = true;
    return true;
#else
    (void)channel; (void)path;
    return false;
#endif
}

void AudioManager::backendSetVolume(AudioChannel channel, float volume) {
#ifdef LIVINGISLAND_USE_MINIAUDIO
    if (g_engineReady && g_soundValid[channel]) {
        ma_sound_set_volume(&g_sounds[channel], volume);
    }
#else
    (void)channel; (void)volume;
#endif
}

// ---------------------------------------------------------------------------
bool AudioManager::initialize(const std::string& assetDirectory) {
    // Code::Blocks normally starts from the project directory, while the
    // Windows build script intentionally starts the executable from /bin.
    // Resolve the asset directory instead of assuming one particular CWD.
    const std::string candidates[] = {
        assetDirectory,
        std::string("../") + assetDirectory,
        std::string("../LiveNature/") + assetDirectory
    };

    m_assetDirectory.clear();
    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
        std::ifstream probe((candidates[i] + "ambient_day.wav").c_str(),
                            std::ios::binary);
        if (probe.good()) {
            m_assetDirectory = candidates[i];
            break;
        }
    }

    if (m_assetDirectory.empty()) {
        // Keep the requested path in the log if the assets genuinely cannot
        // be found; the backend will report each missing file below.
        m_assetDirectory = assetDirectory;
        util::logWarning("Audio assets were not found from the current working directory.");
    } else {
        util::logInfo(("Audio asset directory: " + m_assetDirectory).c_str());
    }

    util::logInfo("Initializing audio...");

    if (!backendInitialize()) {
        util::logWarning("No audio backend compiled in - the world will run silently.");
        util::logInfo("  (See the README: define LIVINGISLAND_USE_MINIAUDIO to enable sound.)");
        m_backendReady = false;
        return true;   // NOT a failure: audio is optional by design
    }

    m_backendReady = true;

    // Each file is optional. A missing one disables that layer and nothing
    // else - exactly what section 33 asks for.
    struct FileEntry { AudioChannel channel; const char* filename; };
    const FileEntry files[] = {
        { AUDIO_AMBIENT_DAY,   "ambient_day.wav"   },
        { AUDIO_AMBIENT_NIGHT, "ambient_night.wav" },
        { AUDIO_RAIN,          "rain.wav"          },
        { AUDIO_WIND,          "wind.wav"          },
        { AUDIO_CAMPFIRE,      "campfire.wav"      },
        { AUDIO_MUSIC,         "music.wav"         }
    };

    int loadedCount = 0;
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); ++i) {
        std::string path = m_assetDirectory + files[i].filename;
        if (backendLoadLooping(files[i].channel, path)) {
            m_loaded[files[i].channel] = true;
            ++loadedCount;
        } else {
            char buffer[256];
            std::snprintf(buffer, sizeof(buffer),
                          "%s unavailable - continuing without that layer.",
                          files[i].filename);
            util::logWarning(buffer);
        }
    }

    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "Audio ready: %d of %d layers loaded.",
                  loadedCount, static_cast<int>(AUDIO_CHANNEL_COUNT));
    util::logInfo(buffer);
    return true;
}

void AudioManager::shutdown() {
    if (m_backendReady) {
        backendShutdown();
        m_backendReady = false;
        util::logInfo("Audio shut down.");
    }
}

void AudioManager::setMasterVolume(float volume) {
    m_masterVolume = util::clampf(volume, 0.0f, 1.0f);
}

void AudioManager::setMuted(bool muted) {
    m_muted = muted;
    util::logInfo(muted ? "Audio muted." : "Audio unmuted.");
}

float AudioManager::attenuation(float distance, float fullVolumeRadius, float silenceRadius) {
    if (distance <= fullVolumeRadius) return 1.0f;
    if (distance >= silenceRadius)    return 0.0f;
    float t = (distance - fullVolumeRadius) / (silenceRadius - fullVolumeRadius);
    return 1.0f - (t * t * (3.0f - 2.0f * t));   // smooth rolloff
}

// ---------------------------------------------------------------------------
//  The mix. This runs whether or not a backend exists, so the levels are
//  always correct and the debug overlay always has something real to show.
// ---------------------------------------------------------------------------
void AudioManager::update(float deltaTime, const AudioEnvironment& environment) {
    // --- decide target levels ---------------------------------------------

    // Day and night ambience cross-fade rather than switching.
    float dayMix   = util::smoothstepf(0.15f, 0.55f, environment.daylight);
    float nightMix = 1.0f - dayMix;

    // Rain masks the nature bed: you do not hear birds in a downpour.
    float rainMask = 1.0f - environment.rainIntensity * 0.75f;

    m_targetVolume[AUDIO_AMBIENT_DAY]   = 0.55f * dayMix * rainMask;
    m_targetVolume[AUDIO_AMBIENT_NIGHT] = 0.48f * nightMix * rainMask;

    // Rain scales non-linearly: a drizzle should be barely there, a downpour
    // should dominate.
    float rain = environment.rainIntensity;
    m_targetVolume[AUDIO_RAIN] = rain * rain * 0.85f + rain * 0.15f;

    // Wind only becomes audible once it is actually blowing.
    m_targetVolume[AUDIO_WIND] =
        util::smoothstepf(0.18f, 1.0f, environment.windStrength) * 0.55f;

    // Campfire: positional, and dies with the fire.
    m_targetVolume[AUDIO_CAMPFIRE] =
        attenuation(environment.campfireDistance, 3.0f, 26.0f)
        * 0.70f * util::clampf(environment.campfireGlow, 0.0f, 1.0f);

    m_targetVolume[AUDIO_MUSIC] = 0.22f;

    // --- ease toward them --------------------------------------------------
    // Rain and wind move faster than the ambience beds, so a weather change is
    // heard before it is fully seen.
    const float rates[AUDIO_CHANNEL_COUNT] = { 0.8f, 0.8f, 1.6f, 1.2f, 3.0f, 0.5f };

    float master = m_muted ? 0.0f : m_masterVolume;

    for (int i = 0; i < AUDIO_CHANNEL_COUNT; ++i) {
        m_currentVolume[i] = util::damp(m_currentVolume[i], m_targetVolume[i],
                                        rates[i], deltaTime);
        if (m_backendReady && m_loaded[i]) {
            backendSetVolume(static_cast<AudioChannel>(i), m_currentVolume[i] * master);
        }
    }
}

float AudioManager::channelVolume(AudioChannel channel) const {
    if (channel < 0 || channel >= AUDIO_CHANNEL_COUNT) return 0.0f;
    return m_currentVolume[channel] * (m_muted ? 0.0f : m_masterVolume);
}
