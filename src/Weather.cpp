#include "Weather.h"
#include "Config.h"

Weather::Weather()
    : m_type(WEATHER_CLEAR)
    , m_rain(0.0f),  m_rainTarget(0.0f)
    , m_wind(0.30f), m_windTarget(0.30f)
    , m_cloud(0.20f), m_cloudTarget(0.20f)
    , m_gustWind(0.30f)
    , m_windPhase(0.0f)
    , m_time(0.0f)
    , m_autoCycle(false)
    , m_autoTimer(0.0f)
    , m_autoInterval(55.0f)
    , m_rng(Config::WORLD_SEED ^ 0x5EED1234u)
{
}

void Weather::setType(WeatherType type) {
    if (type < 0 || type >= WEATHER_TYPE_COUNT) return;
    m_type = type;

    switch (type) {
        case WEATHER_CLEAR:
            m_rainTarget = 0.0f;  m_cloudTarget = 0.12f; m_windTarget = 0.25f; break;
        case WEATHER_CLOUDY:
            m_rainTarget = 0.0f;  m_cloudTarget = 0.70f; m_windTarget = 0.45f; break;
        case WEATHER_RAIN:
            m_rainTarget = 0.45f; m_cloudTarget = 0.80f; m_windTarget = 0.55f; break;
        case WEATHER_HEAVY_RAIN:
            m_rainTarget = 1.0f;  m_cloudTarget = 1.0f;  m_windTarget = 0.92f; break;
        default: break;
    }
}

void Weather::overrideRain(float value) {
    m_rain = m_rainTarget = util::clampf(value, 0.0f, 1.0f);
    m_type = impliedType();
}

void Weather::overrideWind(float value) {
    m_wind = m_windTarget = util::clampf(value, 0.0f, 1.0f);
}

void Weather::overrideCloud(float value) {
    m_cloud = m_cloudTarget = util::clampf(value, 0.0f, 1.0f);
    m_type = impliedType();
}

WeatherType Weather::impliedType() const {
    if (m_rain  > 0.60f) return WEATHER_HEAVY_RAIN;
    if (m_rain  > 0.05f) return WEATHER_RAIN;
    if (m_cloud > 0.50f) return WEATHER_CLOUDY;
    return WEATHER_CLEAR;
}

const char* Weather::name() const {
    switch (impliedType()) {
        case WEATHER_CLEAR:      return "Clear";
        case WEATHER_CLOUDY:     return "Cloudy";
        case WEATHER_RAIN:       return "Rain";
        case WEATHER_HEAVY_RAIN: return "Heavy Rain";
        default:                 return "Clear";
    }
}

void Weather::update(float deltaTime) {
    m_time += deltaTime;

    if (m_autoCycle) {
        m_autoTimer += deltaTime;
        if (m_autoTimer >= m_autoInterval) {
            m_autoTimer = 0.0f;
            m_autoInterval = m_rng.range(45.0f, 90.0f);
            setType(static_cast<WeatherType>(m_rng.rangeInt(0, WEATHER_TYPE_COUNT - 1)));
        }
    }

    // Ease toward the targets. Rain ramps a little faster than cloud cover,
    // which is roughly how a shower behaves.
    m_rain  = util::damp(m_rain,  m_rainTarget,  0.75f, deltaTime);
    m_cloud = util::damp(m_cloud, m_cloudTarget, 0.50f, deltaTime);
    m_wind  = util::damp(m_wind,  m_windTarget,  0.60f, deltaTime);

    // Gusts: two out-of-phase sine waves plus drifting noise. Integrating the
    // phase (rather than multiplying time by the current wind) means a change
    // in wind strength never causes the whole world to snap to a new position.
    float slow = std::sin(m_time * 0.37f) * 0.5f + 0.5f;
    float fast = std::sin(m_time * 1.13f + 1.7f) * 0.5f + 0.5f;
    float drift = util::valueNoise2D(m_time * 0.25f, 3.5f, 91u);

    float gustAmount = (slow * 0.5f + fast * 0.25f + drift * 0.25f);
    m_gustWind = util::clampf(m_wind * (0.70f + 0.55f * gustAmount), 0.0f, 1.35f);

    m_windPhase += deltaTime * (0.6f + m_gustWind * 2.4f);
}
