// ============================================================================
//  Water.h - the sea ring around the island.
// ============================================================================

#ifndef LIVINGISLAND_WATER_H
#define LIVINGISLAND_WATER_H

#include "GLIncludes.h"
#include "Utilities.h"

class Terrain;

class Water {
public:
    Water();

    void initialize(float extent, int resolution);
    void update(float deltaTime, float windStrength);

    void render(const util::Vec3& cameraPosition,
                const util::Vec3& waterColor,
                const util::Vec3& sunDirection,
                const util::Vec3& sunColor,
                float daylight,
                float alpha,
                const Terrain& terrain) const;

    float level() const { return m_level; }

private:
    float surfaceHeight(float x, float z) const;

    util::Vec3 surfaceNormal(float x, float z) const;

    float m_level;
    float m_extent;
    int   m_resolution;
    float m_cellSize;
    float m_time;
    float m_waveAmplitude;
};

#endif // LIVINGISLAND_WATER_H