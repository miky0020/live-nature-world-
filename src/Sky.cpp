#include "Sky.h"
#include "Config.h"
#include "Primitives.h"

using util::Vec3;

Sky::Sky()
    : m_segments(32)
    , m_radius(Config::CAM_FAR_PLANE * 0.55f)
    , m_cloudAltitude(74.0f)
    , m_cloudFieldRadius(260.0f)
    , m_puffList(0)
    , m_built(false)
    , m_time(0.0f)
{
}

Sky::~Sky() {}

void Sky::initialize(int rings, int segments) {
    if (rings < 4) rings = 4;
    if (segments < 8) segments = 8;

    m_segments = segments;
    m_radius   = Config::CAM_FAR_PLANE * 0.55f;

    // ---- dome rings -------------------------------------------------------
    m_rings.clear();
    m_rings.reserve(static_cast<size_t>(rings + 1));

    // Start below the horizon so there is never a gap under the terrain edge,
    // and bias the spacing toward the horizon where the gradient is strongest.
    const float startAngle = -0.16f * util::PI;
    const float endAngle   =  0.5f  * util::PI;

    for (int i = 0; i <= rings; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(rings);
        float shaped = t * t;
        float angle  = util::lerpf(startAngle, endAngle, shaped);

        Ring ring;
        ring.y      = std::sin(angle);
        ring.radius = std::cos(angle);
        ring.blend  = util::clampf(std::sin(angle) / 0.9f, 0.0f, 1.0f);
        ring.blend  = std::sqrt(ring.blend);
        m_rings.push_back(ring);
    }

    m_cosTable.resize(static_cast<size_t>(m_segments + 1));
    m_sinTable.resize(static_cast<size_t>(m_segments + 1));
    for (int s = 0; s <= m_segments; ++s) {
        float a = (static_cast<float>(s) / static_cast<float>(m_segments)) * util::TWO_PI;
        m_cosTable[static_cast<size_t>(s)] = std::cos(a);
        m_sinTable[static_cast<size_t>(s)] = std::sin(a);
    }

    // ---- stars ------------------------------------------------------------
    // Distributed over the upper hemisphere only, and biased away from the
    // horizon where haze would hide them anyway.
    util::Rng rng(Config::WORLD_SEED ^ 0x57A45u);
    m_stars.clear();
    m_stars.reserve(static_cast<size_t>(Config::STAR_COUNT));

    for (int i = 0; i < Config::STAR_COUNT; ++i) {
        float theta = rng.range(0.0f, util::TWO_PI);
        float height = rng.nextFloat();
        height = height * height;              // cluster toward the zenith
        float y = util::lerpf(0.06f, 1.0f, height);
        float r = std::sqrt(util::clampf(1.0f - y * y, 0.0f, 1.0f));

        Star star;
        star.direction    = Vec3(std::cos(theta) * r, y, std::sin(theta) * r);
        star.brightness   = rng.range(0.30f, 1.0f);
        star.twinklePhase = rng.range(0.0f, util::TWO_PI);
        star.twinkleRate  = rng.range(0.6f, 2.4f);
        m_stars.push_back(star);
    }

    // ---- clouds -----------------------------------------------------------
    m_clouds.clear();
    m_clouds.reserve(static_cast<size_t>(Config::CLOUD_COUNT));

    for (int i = 0; i < Config::CLOUD_COUNT; ++i) {
        Cloud cloud;
        cloud.position = Vec3(rng.range(-m_cloudFieldRadius, m_cloudFieldRadius),
                              m_cloudAltitude + rng.range(-16.0f, 22.0f),
                              rng.range(-m_cloudFieldRadius, m_cloudFieldRadius));
        cloud.scale      = rng.range(9.0f, 24.0f);
        cloud.speed      = rng.range(0.55f, 1.35f);
        cloud.alphaScale = rng.range(0.70f, 1.0f);
        cloud.puffCount  = rng.rangeInt(4, 7);
        cloud.seed       = static_cast<unsigned>(rng.rangeInt(1, 100000));
        m_clouds.push_back(cloud);
    }

    util::logInfo("Sky initialized (dome, stars, clouds).");
}

void Sky::buildRenderData() {
    if (m_built) return;

    // One puff, reused for every lump of every cloud.
    m_puffList = glGenLists(1);
    if (m_puffList) {
        glNewList(m_puffList, GL_COMPILE);
        prim::drawBlob(1.0f, 8, 6, 777u, 0.16f, 0.60f);
        glEndList();
    }

    m_built = true;
    util::logInfo("Cloud geometry compiled.");
}

void Sky::destroyRenderData() {
    if (m_puffList) glDeleteLists(m_puffList, 1);
    m_puffList = 0;
    m_built = false;
}

void Sky::update(float deltaTime, const RenderContext& context) {
    m_time += deltaTime;

    // Clouds are pushed by the wind and wrap around the field, so a fixed
    // eighteen of them give an endless sky.
    float drift = (0.6f + context.wind * 5.5f) * deltaTime;

    for (size_t i = 0; i < m_clouds.size(); ++i) {
        Cloud& cloud = m_clouds[i];
        cloud.position.x += drift * cloud.speed;
        cloud.position.z += drift * cloud.speed * 0.35f;

        if (cloud.position.x >  m_cloudFieldRadius) cloud.position.x -= m_cloudFieldRadius * 2.0f;
        if (cloud.position.z >  m_cloudFieldRadius) cloud.position.z -= m_cloudFieldRadius * 2.0f;
        if (cloud.position.x < -m_cloudFieldRadius) cloud.position.x += m_cloudFieldRadius * 2.0f;
        if (cloud.position.z < -m_cloudFieldRadius) cloud.position.z += m_cloudFieldRadius * 2.0f;
    }
}

// ---------------------------------------------------------------------------
void Sky::renderDome(const Vec3& cameraPosition,
                     const Vec3& horizonColor,
                     const Vec3& zenithColor) const
{
    if (m_rings.size() < 2) return;

    glPushMatrix();
    glTranslatef(cameraPosition.x, cameraPosition.y, cameraPosition.z);

    for (size_t r = 0; r + 1 < m_rings.size(); ++r) {
        const Ring& lower = m_rings[r];
        const Ring& upper = m_rings[r + 1];

        Vec3 cLower = util::lerpv(horizonColor, zenithColor, lower.blend);
        Vec3 cUpper = util::lerpv(horizonColor, zenithColor, upper.blend);

        glBegin(GL_TRIANGLE_STRIP);
        for (int s = 0; s <= m_segments; ++s) {
            float c  = m_cosTable[static_cast<size_t>(s)];
            float sn = m_sinTable[static_cast<size_t>(s)];

            glColor3f(cLower.x, cLower.y, cLower.z);
            glVertex3f(c * lower.radius * m_radius, lower.y * m_radius,
                       sn * lower.radius * m_radius);

            glColor3f(cUpper.x, cUpper.y, cUpper.z);
            glVertex3f(c * upper.radius * m_radius, upper.y * m_radius,
                       sn * upper.radius * m_radius);
        }
        glEnd();
    }

    glPopMatrix();
}

void Sky::renderStars(const RenderContext& context) const {
    // Fade in as the sun sets rather than switching on, and hide behind cloud.
    float visibility = context.nightFactor * (1.0f - context.cloudDensity * 0.85f);
    visibility = util::smoothstepf(0.12f, 0.65f, visibility);
    if (visibility <= 0.01f) return;

    glPushMatrix();
    glTranslatef(context.cameraPosition.x, context.cameraPosition.y, context.cameraPosition.z);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_POINT_SMOOTH);

    float distance = m_radius * 0.94f;

    for (size_t i = 0; i < m_stars.size(); ++i) {
        const Star& star = m_stars[i];

        float twinkle = 0.75f + 0.25f * std::sin(m_time * star.twinkleRate + star.twinklePhase);
        float alpha = star.brightness * twinkle * visibility;
        if (alpha < 0.02f) continue;

        glPointSize(star.brightness > 0.82f ? 2.4f : 1.5f);
        glBegin(GL_POINTS);
            glColor4f(1.0f, 1.0f, 0.96f, alpha);
            glVertex3f(star.direction.x * distance,
                       star.direction.y * distance,
                       star.direction.z * distance);
        glEnd();
    }

    glPopMatrix();
}

// A camera-facing disc. The modelview rotation is cleared so the quad always
// squarely faces the viewer without any billboard maths.
void Sky::renderCelestialBody(const Vec3& cameraPosition,
                              const Vec3& direction,
                              float angularSize,
                              const Vec3& color,
                              float alpha) const
{
    if (alpha <= 0.01f) return;

    float distance = m_radius * 0.90f;
    Vec3 position = cameraPosition + direction * distance;

    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);

    GLfloat modelview[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
    modelview[0] = 1.0f; modelview[1] = 0.0f; modelview[2]  = 0.0f;
    modelview[4] = 0.0f; modelview[5] = 1.0f; modelview[6]  = 0.0f;
    modelview[8] = 0.0f; modelview[9] = 0.0f; modelview[10] = 1.0f;
    glLoadMatrixf(modelview);

    float radius = distance * angularSize;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Soft halo
    glBegin(GL_TRIANGLE_FAN);
        glColor4f(color.x, color.y, color.z, alpha * 0.30f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glColor4f(color.x, color.y, color.z, 0.0f);
        for (int i = 0; i <= 20; ++i) {
            float a = (static_cast<float>(i) / 20.0f) * util::TWO_PI;
            glVertex3f(std::cos(a) * radius * 2.6f, std::sin(a) * radius * 2.6f, 0.0f);
        }
    glEnd();

    // Bright core
    glBegin(GL_TRIANGLE_FAN);
        glColor4f(color.x, color.y, color.z, alpha);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glColor4f(color.x, color.y, color.z, alpha * 0.75f);
        for (int i = 0; i <= 20; ++i) {
            float a = (static_cast<float>(i) / 20.0f) * util::TWO_PI;
            glVertex3f(std::cos(a) * radius, std::sin(a) * radius, 0.0f);
        }
    glEnd();

    glPopMatrix();
}

void Sky::renderBackdrop(const RenderContext& context,
                         const Vec3& horizonColor,
                         const Vec3& zenithColor,
                         const Vec3& sunDirection) const
{
    // GL_COLOR_BUFFER_BIT is included so glPopAttrib restores the blend
    // FUNCTION as well as whether blending is enabled - renderCelestialBody
    // switches to additive blending for the sun/moon glow below, and without
    // this bit that additive function would leak into every draw call after
    // this function returns (see the longer note in World::render).
    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_POINT_BIT
                | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);   // known starting state

    renderDome(context.cameraPosition, horizonColor, zenithColor);
    renderStars(context);

    float cloudHiding = 1.0f - context.cloudDensity * 0.80f;

    // Sun: visible while it is above the horizon, reddening as it sets.
    if (sunDirection.y > -0.12f) {
        float lowness = 1.0f - util::smoothstepf(0.0f, 0.35f, sunDirection.y);
        Vec3 sunColor = util::lerpv(Vec3(1.0f, 0.97f, 0.85f),
                                    Vec3(1.0f, 0.55f, 0.22f), lowness);
        float alpha = util::smoothstepf(-0.12f, 0.06f, sunDirection.y) * cloudHiding;
        renderCelestialBody(context.cameraPosition, sunDirection, 0.030f, sunColor, alpha);
    }

    // Moon: directly opposite the sun, so it rises as the sun sets.
    Vec3 moonDirection = -sunDirection;
    if (moonDirection.y > -0.10f) {
        float alpha = util::smoothstepf(-0.10f, 0.10f, moonDirection.y)
                    * context.nightFactor * cloudHiding;
        renderCelestialBody(context.cameraPosition, moonDirection, 0.024f,
                            Vec3(0.92f, 0.94f, 1.0f), alpha * 0.9f);
    }

    glDepthMask(GL_TRUE);
    glPopAttrib();
}

// ---------------------------------------------------------------------------
void Sky::renderClouds(const RenderContext& context) const {
    if (!m_built) return;

    // Cloud density controls both how many are drawn and how solid they look.
    float density = context.cloudDensity;
    if (density <= 0.02f) return;

    int visibleLimit = static_cast<int>(std::ceil(density * static_cast<float>(m_clouds.size())));

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT
                | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_FOG);

    // Clouds take their colour from the sky, so they look right at every hour:
    // white at noon, pink at sunset, near-black at midnight.
    Vec3 lit   = util::lerpv(Vec3(0.30f, 0.33f, 0.42f), Vec3(1.0f, 0.99f, 0.97f),
                             context.daylight);
    Vec3 shade = util::lerpv(Vec3(0.16f, 0.18f, 0.26f), Vec3(0.62f, 0.66f, 0.74f),
                             context.daylight);

    // Heavier weather makes them darker and denser.
    lit   = util::lerpv(lit,   lit   * 0.62f, context.rainIntensity);
    shade = util::lerpv(shade, shade * 0.58f, context.rainIntensity);

    float baseAlpha = util::lerpf(0.30f, 0.80f, density);

    for (int i = 0; i < visibleLimit && i < static_cast<int>(m_clouds.size()); ++i) {
        const Cloud& cloud = m_clouds[static_cast<size_t>(i)];

        // Clouds sit far above and around the camera; track the camera in XZ
        // so the sky is never empty behind the player.
        Vec3 position(cloud.position.x + context.cameraPosition.x * 0.0f,
                      cloud.position.y,
                      cloud.position.z);

        if (!context.frustum.sphereVisible(position, cloud.scale * 1.8f)) continue;

        util::Rng rng(cloud.seed);
        float alpha = baseAlpha * cloud.alphaScale;

        glPushMatrix();
        glTranslatef(position.x, position.y, position.z);

        for (int p = 0; p < cloud.puffCount; ++p) {
            float ox = rng.range(-1.0f, 1.0f) * cloud.scale * 0.85f;
            float oy = rng.range(-0.25f, 0.30f) * cloud.scale * 0.5f;
            float oz = rng.range(-1.0f, 1.0f) * cloud.scale * 0.55f;
            float ps = rng.range(0.55f, 1.0f) * cloud.scale * 0.65f;

            // Upper puffs catch the light, lower ones stay in shadow: cheap
            // fake self-shading that reads surprisingly well.
            float lightFactor = util::clampf(0.5f + oy / (cloud.scale * 0.5f), 0.0f, 1.0f);
            Vec3 color = util::lerpv(shade, lit, lightFactor);

            glPushMatrix();
            glTranslatef(ox, oy, oz);
            glScalef(ps, ps * 0.7f, ps);
            glColor4f(color.x, color.y, color.z, alpha);
            if (m_puffList) glCallList(m_puffList);
            glPopMatrix();
        }

        glPopMatrix();
    }

    glDepthMask(GL_TRUE);
    glPopAttrib();
}
