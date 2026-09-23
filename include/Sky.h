// ============================================================================
//  Sky.h - gradient dome, stars, sun, moon and drifting clouds.
//
//  The dome is recentred on the camera every frame so it can never be reached,
//  and it is drawn first with depth writes off, acting as a coloured backdrop
//  the rest of the scene is composited over.
//
//  Ring directions and star positions are precomputed once; only colours and
//  alphas change per frame, so there is no trigonometry in the render loop.
//
//  Clouds drift with the wind and wrap around when they leave the sky, so a
//  fixed number of them covers an unbounded sky (section 20).
// ============================================================================
#ifndef LIVINGISLAND_SKY_H
#define LIVINGISLAND_SKY_H

#include "GLIncludes.h"
#include "RenderContext.h"
#include "Utilities.h"

#include <vector>

class Sky {
public:
    Sky();
    ~Sky();

    void initialize(int rings = 16, int segments = 32);
    void buildRenderData();
    void destroyRenderData();

    void update(float deltaTime, const RenderContext& context);

    // Backdrop: dome, stars, sun and moon. Drawn before everything else.
    void renderBackdrop(const RenderContext& context,
                        const util::Vec3& horizonColor,
                        const util::Vec3& zenithColor,
                        const util::Vec3& sunDirection) const;

    // Clouds are geometry in the world, drawn with the other transparent
    // passes rather than with the backdrop.
    void renderClouds(const RenderContext& context) const;

private:
    struct Ring {
        float y;
        float radius;
        float blend;
    };

    struct Star {
        util::Vec3 direction;
        float brightness;
        float twinklePhase;
        float twinkleRate;
    };

    struct Cloud {
        util::Vec3 position;     // world space, high above the island
        float scale;
        float speed;
        float alphaScale;
        int   puffCount;
        unsigned seed;
    };

    void renderDome(const util::Vec3& cameraPosition,
                    const util::Vec3& horizonColor,
                    const util::Vec3& zenithColor) const;
    void renderStars(const RenderContext& context) const;
    void renderCelestialBody(const util::Vec3& cameraPosition,
                             const util::Vec3& direction,
                             float angularSize,
                             const util::Vec3& color,
                             float alpha) const;

    int   m_segments;
    float m_radius;
    float m_cloudAltitude;
    float m_cloudFieldRadius;

    std::vector<Ring>  m_rings;
    std::vector<float> m_cosTable;
    std::vector<float> m_sinTable;
    std::vector<Star>  m_stars;
    std::vector<Cloud> m_clouds;

    GLuint m_puffList;
    bool   m_built;
    float  m_time;
};

#endif // LIVINGISLAND_SKY_H
