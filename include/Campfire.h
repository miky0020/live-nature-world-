// ============================================================================
//  Campfire.h - logs, stone ring, animated flames, sparks and a warm light.
//
//  Section 9 is explicit that a rotating static flame is not acceptable. The
//  flames here are rebuilt every frame from stacked rings whose radius and
//  centre are displaced by drifting value noise, so the silhouette genuinely
//  changes shape rather than spinning.
//
//  Cost is trivial: three layers of six rings of eight slices.
// ============================================================================
#ifndef LIVINGISLAND_CAMPFIRE_H
#define LIVINGISLAND_CAMPFIRE_H

#include "GLIncludes.h"
#include "ParticleSystem.h"
#include "RenderContext.h"
#include "Utilities.h"

class Terrain;

class Campfire {
public:
    Campfire();
    ~Campfire();

    void initialize(const Terrain& terrain, float x, float z, unsigned seed);
    void buildRenderData();
    void destroyRenderData();

    void update(float deltaTime, const RenderContext& context);

    void renderSolid(const RenderContext& context) const;    // logs and stones
    void renderEffects(const RenderContext& context) const;  // flames and sparks

    // Installs the fire as a positional GL light with attenuation.
    void applyLight(GLenum light, float nightFactor) const;

    util::Vec3 position() const { return m_position; }
    float glow() const { return m_glow; }

private:
    void drawFlameLayer(float baseRadius, float height, float timeOffset,
                        float r, float g, float b, float alpha) const;

    util::Vec3  m_position;
    float       m_time;
    float       m_flicker;   // 0.75 .. 1.25 - drives both light and flame size
    float       m_glow;      // 0..1 overall fire strength
    SparkSystem m_sparks;
    SmokeSystem m_smoke;
    GLuint      m_solidList;
    unsigned    m_seed;
    bool        m_built;
};

#endif // LIVINGISLAND_CAMPFIRE_H
