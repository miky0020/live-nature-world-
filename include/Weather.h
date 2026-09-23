// ============================================================================
//  Weather.h - weather state, and the gradual transitions between states.
//
//  Two layers, which is what makes section 11's "transitions must be gradual"
//  work alongside section 13's "controls should update the world immediately":
//
//    * choosing a preset (keys 1-3) moves the TARGETS, and the current values
//      ease toward them over a couple of seconds
//    * dragging a slider overrides a value outright, because the player is
//      holding the control and expects to see the result under their finger
// ============================================================================
#ifndef LIVINGISLAND_WEATHER_H
#define LIVINGISLAND_WEATHER_H

#include "Utilities.h"

enum WeatherType {
    WEATHER_CLEAR = 0,
    WEATHER_CLOUDY,
    WEATHER_RAIN,
    WEATHER_HEAVY_RAIN,
    WEATHER_TYPE_COUNT
};

class Weather {
public:
    Weather();

    void update(float deltaTime);

    // Preset change - eases in.
    void setType(WeatherType type);

    // Slider control - takes effect at once.
    void overrideRain(float value);
    void overrideWind(float value);
    void overrideCloud(float value);

    void setAutoCycle(bool enabled) { m_autoCycle = enabled; }
    bool autoCycle() const { return m_autoCycle; }

    float rain()  const { return m_rain; }
    float wind()  const { return m_wind; }
    float cloud() const { return m_cloud; }

    // Wind with gusts layered on. Everything that sways should use this rather
    // than the raw value, so the whole world gusts together.
    float gustWind() const { return m_gustWind; }
    float windPhase() const { return m_windPhase; }

    WeatherType type() const { return m_type; }
    const char* name() const;

    // Re-derives the named type from the current values, so the HUD stays
    // honest after the player has been dragging sliders.
    WeatherType impliedType() const;

private:
    WeatherType m_type;

    float m_rain,  m_rainTarget;
    float m_wind,  m_windTarget;
    float m_cloud, m_cloudTarget;

    float m_gustWind;
    float m_windPhase;
    float m_time;

    bool  m_autoCycle;
    float m_autoTimer;
    float m_autoInterval;
    util::Rng m_rng;
};

#endif // LIVINGISLAND_WEATHER_H
