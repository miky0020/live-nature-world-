// ============================================================================
//  Lighting.h - the sun, the ambient term, and the palette everything else
//  colours itself from.
//
//  One rule drives the whole system: a single continuous value (time of day in
//  hours) produces the sun direction, and the sun's elevation produces every
//  colour. Nothing switches between discrete "day" and "night" states, so
//  transitions are smooth for free.
//
//  Stage 1 uses a fixed time of day. Stage 6 hands the clock over to the
//  day/night cycle and adds the moon, stars and weather tinting - the
//  interface below is already shaped for it.
// ============================================================================
#ifndef LIVINGISLAND_LIGHTING_H
#define LIVINGISLAND_LIGHTING_H

#include "GLIncludes.h"
#include "Utilities.h"

class Lighting {
public:
    Lighting();

    void initialize();
    void update(float timeOfDayHours, float cloudiness);
    void apply() const;                  // pushes the current sun into GL_LIGHT0

    util::Vec3 sunDirection() const { return m_sunDirection; }   // points FROM the sun
    util::Vec3 sunColor()     const { return m_sunColor; }
    util::Vec3 ambientColor() const { return m_ambientColor; }
    util::Vec3 horizonColor() const { return m_horizonColor; }
    util::Vec3 zenithColor()  const { return m_zenithColor; }
    util::Vec3 fogColor()     const { return m_fogColor; }
    util::Vec3 waterColor()   const { return m_waterColor; }

    float sunElevation() const { return m_sunElevation; }   // -1 .. 1
    float daylight()     const { return m_daylight; }       // 0 at night, 1 at noon

private:
    util::Vec3 m_sunDirection;
    util::Vec3 m_sunColor;
    util::Vec3 m_ambientColor;
    util::Vec3 m_horizonColor;
    util::Vec3 m_zenithColor;
    util::Vec3 m_fogColor;
    util::Vec3 m_waterColor;

    float m_sunElevation;
    float m_daylight;
};

#endif // LIVINGISLAND_LIGHTING_H
