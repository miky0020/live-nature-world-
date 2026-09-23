#include "Lighting.h"
#include "Config.h"

using util::Vec3;

Lighting::Lighting()
    : m_sunDirection(0.4f, 0.8f, 0.3f)
    , m_sunColor(1.0f, 0.97f, 0.90f)
    , m_ambientColor(0.32f, 0.35f, 0.40f)
    , m_horizonColor(0.62f, 0.76f, 0.88f)
    , m_zenithColor(0.22f, 0.45f, 0.78f)
    , m_fogColor(0.66f, 0.76f, 0.85f)
    , m_waterColor(0.16f, 0.36f, 0.48f)
    , m_sunElevation(1.0f)
    , m_daylight(1.0f)
{
}

void Lighting::initialize() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Vertex colours drive the diffuse/ambient material, which keeps the code
    // free of per-object glMaterial churn.
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Kept low deliberately. Raising it to give the sea a glint washed out
    // every other surface in the scene, so the water computes its own
    // highlight per vertex instead (see Water::render).
    const GLfloat specular[] = { 0.10f, 0.10f, 0.10f, 1.0f };
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

    util::logInfo("Lighting system initialized (sun on GL_LIGHT0).");
}

void Lighting::update(float timeOfDayHours, float cloudiness) {
    cloudiness = util::clampf(cloudiness, 0.0f, 1.0f);

    // Map 0..24h onto a full circle, with the sun at its highest at 12:00.
    float angle = (timeOfDayHours / 24.0f) * util::TWO_PI - util::PI * 0.5f;

    m_sunElevation = std::sin(angle);
    float horizontal = std::cos(angle);

    // Tilt the arc slightly so the sun does not track a perfectly flat line.
    m_sunDirection = Vec3(horizontal * 0.86f, m_sunElevation, horizontal * 0.34f + 0.18f)
                         .normalized();

    // 0 well before sunrise, 1 in full daylight. The soft edge is the reason
    // dawn and dusk feel gradual rather than switched.
    m_daylight = util::smoothstepf(-0.12f, 0.26f, m_sunElevation);

    // Warm, low-angle light near the horizon; neutral white overhead.
    float warmth = 1.0f - util::smoothstepf(0.02f, 0.38f, m_sunElevation);

    const Vec3 noonSun  (1.00f, 0.97f, 0.90f);
    const Vec3 goldenSun(1.00f, 0.63f, 0.34f);
    const Vec3 moonLight(0.34f, 0.42f, 0.62f);

    Vec3 daySun = util::lerpv(noonSun, goldenSun, warmth);
    m_sunColor  = util::lerpv(moonLight, daySun, m_daylight);
    m_sunColor  = util::lerpv(m_sunColor, m_sunColor * 0.72f, cloudiness * 0.6f);

    const Vec3 nightAmbient(0.10f, 0.13f, 0.22f);
    const Vec3 dayAmbient  (0.27f, 0.29f, 0.33f);
    m_ambientColor = util::lerpv(nightAmbient, dayAmbient, m_daylight);
    m_ambientColor = util::lerpv(m_ambientColor, m_ambientColor * 1.15f, cloudiness * 0.4f);

    // --- sky palette -------------------------------------------------------
    const Vec3 nightZenith (0.03f, 0.05f, 0.14f);
    const Vec3 nightHorizon(0.08f, 0.11f, 0.21f);
    const Vec3 duskZenith  (0.20f, 0.22f, 0.44f);
    const Vec3 duskHorizon (0.92f, 0.52f, 0.30f);
    const Vec3 dayZenith   (0.20f, 0.44f, 0.80f);
    const Vec3 dayHorizon  (0.66f, 0.80f, 0.92f);

    float duskBlend = 1.0f - std::fabs(util::clampf(m_sunElevation / 0.30f, -1.0f, 1.0f));
    duskBlend = util::clampf(duskBlend, 0.0f, 1.0f);

    Vec3 zenith  = util::lerpv(nightZenith,  dayZenith,  m_daylight);
    Vec3 horizon = util::lerpv(nightHorizon, dayHorizon, m_daylight);

    m_zenithColor  = util::lerpv(zenith,  duskZenith,  duskBlend * 0.75f);
    m_horizonColor = util::lerpv(horizon, duskHorizon, duskBlend * 0.85f);

    const Vec3 overcast(0.55f, 0.57f, 0.60f);
    m_zenithColor  = util::lerpv(m_zenithColor,  overcast * 0.75f, cloudiness * 0.65f);
    m_horizonColor = util::lerpv(m_horizonColor, overcast,         cloudiness * 0.65f);

    // Fog matches the horizon, which is what sells aerial perspective.
    m_fogColor = util::lerpv(m_horizonColor, m_zenithColor, 0.25f);

    const Vec3 nightWater(0.04f, 0.09f, 0.17f);
    const Vec3 dayWater  (0.14f, 0.38f, 0.50f);
    m_waterColor = util::lerpv(nightWater, dayWater, m_daylight);
    m_waterColor = util::lerpv(m_waterColor, m_horizonColor * 0.55f, 0.28f);
}

void Lighting::apply() const {
    // Re-asserted every frame on purpose. Vertex colours only reach the
    // material through GL_COLOR_MATERIAL; if anything in the frame leaves it
    // disabled, every lit surface silently falls back to OpenGL's default grey
    // material and the whole world renders white. Setting it once at startup
    // makes that failure mode possible - setting it per frame makes it
    // impossible, and costs nothing.
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Non-water surfaces must not pick up a specular term.
    const GLfloat noSpecular[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, noSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);

    // A directional light: w = 0 means "infinitely far away in this direction".
    const GLfloat position[] = { m_sunDirection.x, m_sunDirection.y, m_sunDirection.z, 0.0f };
    const GLfloat diffuse[]  = { m_sunColor.x, m_sunColor.y, m_sunColor.z, 1.0f };
    const GLfloat ambient[]  = { m_ambientColor.x, m_ambientColor.y, m_ambientColor.z, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
}
