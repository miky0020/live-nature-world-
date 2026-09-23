#include "Campfire.h"
#include "Config.h"
#include "Primitives.h"
#include "Terrain.h"

using util::Vec3;

Campfire::Campfire()
    : m_position(0.0f, 0.0f, 0.0f)
    , m_time(0.0f)
    , m_flicker(1.0f)
    , m_glow(1.0f)
    , m_solidList(0)
    , m_seed(1u)
    , m_built(false)
{
}

Campfire::~Campfire() {}

void Campfire::initialize(const Terrain& terrain, float x, float z, unsigned seed) {
    m_seed = seed;
    float ground = terrain.heightAt(x, z);
    m_position = Vec3(x, ground, z);

    m_sparks.initialize(Config::MAX_SPARK_PARTICLES);
    m_sparks.setOrigin(m_position + Vec3(0.0f, 0.35f, 0.0f));

    m_smoke.initialize(Config::MAX_SMOKE_PARTICLES);
    m_smoke.setOrigin(m_position + Vec3(0.0f, 0.9f, 0.0f));

    util::logInfo("Campfire placed.");
}

void Campfire::buildRenderData() {
    if (m_built) return;

    m_solidList = glGenLists(1);
    if (!m_solidList) {
        util::logWarning("Could not allocate the campfire display list.");
        return;
    }

    util::Rng rng(m_seed ^ 0xF12Eu);

    glNewList(m_solidList, GL_COMPILE);

    // --- stone ring --------------------------------------------------------
    const int stoneCount = 11;
    for (int i = 0; i < stoneCount; ++i) {
        float angle = (static_cast<float>(i) / static_cast<float>(stoneCount)) * util::TWO_PI;
        float radius = 1.05f + rng.range(-0.08f, 0.08f);
        float scale = rng.range(0.20f, 0.32f);

        glPushMatrix();
        glTranslatef(std::cos(angle) * radius, scale * 0.35f, std::sin(angle) * radius);
        glRotatef(rng.range(0.0f, 360.0f), 0.0f, 1.0f, 0.0f);
        glScalef(scale, scale * 0.75f, scale);
        float shade = rng.range(0.34f, 0.52f);
        glColor3f(shade, shade * 0.97f, shade * 0.92f);
        prim::drawBlob(1.0f, 6, 4, 300u + static_cast<unsigned>(i) * 17u, 0.28f, 0.8f);
        glPopMatrix();
    }

    // --- ash bed -----------------------------------------------------------
    glColor3f(0.17f, 0.15f, 0.14f);
    glPushMatrix();
    glTranslatef(0.0f, 0.02f, 0.0f);
    prim::drawCylinder(0.92f, 0.86f, 0.06f, 12, false, true);
    glPopMatrix();

    // --- logs, leaned into a tepee ----------------------------------------
    const int logCount = 5;
    for (int i = 0; i < logCount; ++i) {
        float angle = (static_cast<float>(i) / static_cast<float>(logCount)) * 360.0f
                    + rng.range(-12.0f, 12.0f);

        glPushMatrix();
        glRotatef(angle, 0.0f, 1.0f, 0.0f);
        glTranslatef(0.42f, 0.04f, 0.0f);
        glRotatef(-62.0f, 0.0f, 0.0f, 1.0f);

        // Charred at the base, bare wood further out.
        glColor3f(0.20f, 0.14f, 0.10f);
        prim::drawCylinder(0.10f, 0.085f, 0.55f, 6, true, false);
        glColor3f(0.34f, 0.24f, 0.15f);
        glTranslatef(0.0f, 0.55f, 0.0f);
        prim::drawCylinder(0.085f, 0.07f, 0.65f, 6, false, true);
        glPopMatrix();
    }

    glEndList();
    m_built = true;
    util::logInfo("Campfire geometry compiled.");
}

void Campfire::destroyRenderData() {
    if (m_solidList) glDeleteLists(m_solidList, 1);
    m_solidList = 0;
    m_built = false;
}

void Campfire::update(float deltaTime, const RenderContext& context) {
    m_time += deltaTime;

    // Flicker from two noise samples at different rates: one slow surge, one
    // fast jitter. A single sine would read as a pulse, not a fire.
    float slow = util::valueNoise2D(m_time * 1.7f, 0.0f, m_seed);
    float fast = util::valueNoise2D(m_time * 6.3f, 11.0f, m_seed + 5u);
    m_flicker = 0.80f + slow * 0.26f + fast * 0.14f;

    // Rain dampens the fire; wind makes it flare.
    float rainDamping = 1.0f - context.rainIntensity * 0.55f;
    m_glow = util::clampf(m_flicker * rainDamping * (0.9f + context.wind * 0.25f),
                          0.15f, 1.5f);

    m_sparks.setOrigin(m_position + Vec3(0.0f, 0.45f, 0.0f));
    m_smoke.setOrigin(m_position + Vec3(0.0f, 0.9f, 0.0f));

    float emission = 14.0f * m_glow * (1.0f - context.rainIntensity * 0.6f);
    m_sparks.update(deltaTime, context, emission);

    // A damp fire smokes more, not less - rain suppresses the flame (m_glow)
    // but adds to the smoke rather than subtracting from it.
    float smokeEmission = 4.5f * m_glow + context.rainIntensity * 6.0f;
    m_smoke.update(deltaTime, context, smokeEmission);
}

void Campfire::applyLight(GLenum light, float nightFactor) const {
    // Warm point light just above the logs. Quadratic attenuation keeps its
    // influence local, which matters because fixed-function lighting is
    // per-vertex and a wide radius would smear across the whole terrain.
    const GLfloat position[] = { m_position.x, m_position.y + 0.55f, m_position.z, 1.0f };

    // The fire is always lit, but it only *reads* at dusk and after dark.
    float strength = m_glow * (0.28f + nightFactor * 1.45f);

    const GLfloat diffuse[] = { 1.00f * strength, 0.58f * strength, 0.22f * strength, 1.0f };
    const GLfloat ambient[] = { 0.16f * strength, 0.07f * strength, 0.02f * strength, 1.0f };

    glLightfv(light, GL_POSITION, position);
    glLightfv(light, GL_DIFFUSE,  diffuse);
    glLightfv(light, GL_AMBIENT,  ambient);
    glLightf(light, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf(light, GL_LINEAR_ATTENUATION,    0.055f);
    glLightf(light, GL_QUADRATIC_ATTENUATION, 0.016f);
    glEnable(light);
}

void Campfire::renderSolid(const RenderContext& context) const {
    if (!m_built) return;
    if (!context.frustum.sphereVisible(m_position + Vec3(0.0f, 0.6f, 0.0f), 2.2f)) return;

    ++context.visibleProps;

    glPushMatrix();
    glTranslatef(m_position.x, m_position.y, m_position.z);
    glCallList(m_solidList);
    glPopMatrix();
}

// One cone-ish flame layer. Each horizontal ring is offset and resized by
// noise sampled at (height, time), which is what makes the shape crawl.
void Campfire::drawFlameLayer(float baseRadius, float height, float timeOffset,
                              float r, float g, float b, float alpha) const
{
    const int rings = 7;
    const int slices = 9;

    for (int ring = 0; ring < rings; ++ring) {
        float t0 = static_cast<float>(ring)     / static_cast<float>(rings);
        float t1 = static_cast<float>(ring + 1) / static_cast<float>(rings);

        float y0 = t0 * height;
        float y1 = t1 * height;

        // Taper toward the tip, with noise on top.
        float n0 = util::valueNoise2D(t0 * 3.1f, m_time * 2.6f + timeOffset, m_seed + 9u);
        float n1 = util::valueNoise2D(t1 * 3.1f, m_time * 2.6f + timeOffset, m_seed + 9u);

        float r0 = baseRadius * (1.0f - t0 * t0) * (0.70f + n0 * 0.60f);
        float r1 = baseRadius * (1.0f - t1 * t1) * (0.70f + n1 * 0.60f);

        // Lateral drift so the flame licks sideways rather than standing still.
        float sway0 = (util::valueNoise2D(m_time * 2.2f + timeOffset, t0 * 4.0f, m_seed + 21u) - 0.5f)
                    * t0 * baseRadius * 1.5f;
        float sway1 = (util::valueNoise2D(m_time * 2.2f + timeOffset, t1 * 4.0f, m_seed + 21u) - 0.5f)
                    * t1 * baseRadius * 1.5f;

        float swayZ0 = (util::valueNoise2D(m_time * 1.9f + timeOffset + 7.0f, t0 * 4.0f, m_seed + 33u) - 0.5f)
                     * t0 * baseRadius * 1.2f;
        float swayZ1 = (util::valueNoise2D(m_time * 1.9f + timeOffset + 7.0f, t1 * 4.0f, m_seed + 33u) - 0.5f)
                     * t1 * baseRadius * 1.2f;

        // Hotter and more opaque at the base, fading out at the tip.
        float a0 = alpha * (1.0f - t0 * 0.85f);
        float a1 = alpha * (1.0f - t1 * 0.85f);

        glBegin(GL_TRIANGLE_STRIP);
        for (int s = 0; s <= slices; ++s) {
            float angle = (static_cast<float>(s) / static_cast<float>(slices)) * util::TWO_PI;
            float c = std::cos(angle), sn = std::sin(angle);

            glColor4f(r, g * (1.0f - t0 * 0.45f), b, a0);
            glVertex3f(sway0 + c * r0, y0, swayZ0 + sn * r0);

            glColor4f(r, g * (1.0f - t1 * 0.45f), b, a1);
            glVertex3f(sway1 + c * r1, y1, swayZ1 + sn * r1);
        }
        glEnd();
    }
}

void Campfire::renderEffects(const RenderContext& context) const {
    if (!context.frustum.sphereVisible(m_position + Vec3(0.0f, 1.2f, 0.0f), 3.5f)) return;

    // GL_COLOR_BUFFER_BIT included: the additive blend function set below
    // must not survive past this function's glPopAttrib.
    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT
                | GL_COLOR_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);     // additive: flames add light
    glDepthMask(GL_FALSE);

    glPushMatrix();
    glTranslatef(m_position.x, m_position.y + 0.12f, m_position.z);

    float scale = m_glow;
    float rainFade = 1.0f - context.rainIntensity * 0.45f;

    // Three nested layers: a wide dull red base, an orange body, a small hot
    // yellow core. Different time offsets keep them out of sync.
    drawFlameLayer(0.62f * scale, 1.05f * scale, 0.0f,
                   0.95f, 0.30f, 0.08f, 0.34f * rainFade);
    drawFlameLayer(0.44f * scale, 1.45f * scale, 3.7f,
                   1.00f, 0.55f, 0.12f, 0.46f * rainFade);
    drawFlameLayer(0.24f * scale, 1.75f * scale, 8.2f,
                   1.00f, 0.88f, 0.42f, 0.58f * rainFade);

    glPopMatrix();

    // A few hot ember cores make the base look like the reference instead of
    // reading as one flat orange volume.
    glPointSize(3.5f);
    glBegin(GL_POINTS);
        for (int i = 0; i < 9; ++i) {
            float a = (static_cast<float>(i) / 9.0f) * util::TWO_PI
                    + m_time * (0.25f + 0.03f * i);
            float r = 0.18f + 0.045f * static_cast<float>(i % 3);
            float flick = 0.55f + 0.45f
                        * std::sin(m_time * (2.2f + i * 0.11f) + i);
            glColor4f(1.0f, 0.20f + flick * 0.45f, 0.03f, 0.35f + flick * 0.45f);
            glVertex3f(std::cos(a) * r, 0.18f + 0.08f * flick,
                       std::sin(a) * r);
        }
    glEnd();

    // Warm glow on the ground under the fire, strongest at night.
    float groundGlow = m_glow * (0.10f + context.nightFactor * 0.40f) * rainFade;
    if (groundGlow > 0.01f) {
        glPushMatrix();
        glTranslatef(m_position.x, m_position.y + 0.05f, m_position.z);
        glBegin(GL_TRIANGLE_FAN);
            glColor4f(1.0f, 0.55f, 0.18f, groundGlow);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor4f(1.0f, 0.40f, 0.10f, 0.0f);
            for (int i = 0; i <= 16; ++i) {
                float angle = (static_cast<float>(i) / 16.0f) * util::TWO_PI;
                glVertex3f(std::cos(angle) * 4.2f, 0.0f, std::sin(angle) * 4.2f);
            }
        glEnd();
        glPopMatrix();
    }

    glPopAttrib();

    m_sparks.render(context);
    m_smoke.render(context);
}
