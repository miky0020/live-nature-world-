#include "Environment.h"
#include "Config.h"
#include "Primitives.h"
#include "Terrain.h"

using util::Vec3;

// Wood palette, shared so the treehouse and the benches look like they came
// from the same island.
static const Vec3 WOOD_DARK  (0.32f, 0.22f, 0.14f);
static const Vec3 WOOD_MID   (0.46f, 0.33f, 0.20f);
static const Vec3 WOOD_LIGHT (0.58f, 0.44f, 0.28f);
static const Vec3 WOOD_ROOF  (0.38f, 0.26f, 0.19f);

Environment::Environment()
    : m_platformHeight(5.4f)
    , m_lanternFlicker(1.0f)
    , m_time(0.0f)
    , m_treehouseList(0)
    , m_benchList(0)
    , m_logSeatList(0)
    , m_seed(1u)
    , m_built(false)
{
}

Environment::~Environment() {}

void Environment::initialize(const Terrain& terrain, unsigned seed) {
    m_seed = seed;

    float ground = terrain.heightAt(Config::TREEHOUSE_X, Config::TREEHOUSE_Z);
    m_treehousePosition = Vec3(Config::TREEHOUSE_X, ground, Config::TREEHOUSE_Z);
    m_platformHeight = 5.4f;

    m_lanternPosition = m_treehousePosition
                      + Vec3(1.65f, m_platformHeight + 1.1f, 1.45f);

    // --- seating around the campfire ---------------------------------------
    util::Rng rng(seed ^ 0xB3A7u);
    m_props.clear();

    const int seatCount = 5;
    for (int i = 0; i < seatCount; ++i) {
        float angle = (static_cast<float>(i) / static_cast<float>(seatCount)) * util::TWO_PI
                    + 0.35f;
        float radius = 2.75f;

        float x = Config::CAMPFIRE_X + std::cos(angle) * radius;
        float z = Config::CAMPFIRE_Z + std::sin(angle) * radius;

        Prop prop;
        prop.position = Vec3(x, terrain.heightAt(x, z), z);
        // Face the fire.
        prop.rotation = -util::toDegrees(angle) + 90.0f;
        prop.scale    = rng.range(0.92f, 1.10f);
        prop.type     = (i % 2 == 0) ? 1 : 0;
        prop.color    = util::lerpv(WOOD_DARK, WOOD_LIGHT, rng.nextFloat());
        m_props.push_back(prop);
    }

    // --- lantern posts along the ring path ----------------------------------
    const int lanternCount = 6;
    for (int i = 0; i < lanternCount; ++i) {
        float angle = (static_cast<float>(i) / static_cast<float>(lanternCount)) * util::TWO_PI;
        float x = std::cos(angle) * (Config::PATH_RING_RADIUS + 1.6f);
        float z = std::sin(angle) * (Config::PATH_RING_RADIUS + 1.6f);

        Prop prop;
        prop.position = Vec3(x, terrain.heightAt(x, z), z);
        prop.rotation = rng.range(0.0f, 360.0f);
        prop.scale    = 1.0f;
        prop.type     = 2;
        prop.color    = WOOD_MID;
        m_props.push_back(prop);
    }

    util::logInfo("Environment landmarks placed (treehouse, seating, lantern posts).");
}

// ---------------------------------------------------------------------------
//  Treehouse construction
// ---------------------------------------------------------------------------
void Environment::buildTreehouse() {
    const float platformY = m_platformHeight;
    const float platformW = 4.4f;
    const float platformD = 3.8f;

    // --- host tree ---------------------------------------------------------
    // A thicker, taller trunk than the forest species, so the treehouse reads
    // as built around a specific landmark tree.
    // Everything structural starts BELOW y=0. The treehouse sits on a slope,
    // and a trunk that stops exactly at the sampled ground height leaves the
    // whole landmark visibly hovering on the downhill side. Burying the base
    // costs nothing and removes the floating entirely (section 29).
    const float BURY = 3.0f;

    glColor3f(0.30f, 0.21f, 0.14f);
    glPushMatrix();
    glTranslatef(0.0f, -BURY, 0.0f);
    prim::drawCylinder(0.86f, 0.46f, 9.5f + BURY, 12, false, false);
    glPopMatrix();

    // An earth mound at the base. Geometrically the trunk is already buried
    // well below the surrounding ground, but with no shadows the eye has no
    // contact cue and reads the structure as hovering. A small dome of soil
    // that visibly meets the slope fixes the perception, which is the part
    // that actually matters.
    glColor3f(0.34f, 0.27f, 0.18f);
    glPushMatrix();
    glTranslatef(0.0f, -0.55f, 0.0f);
    prim::drawBlob(2.30f, 12, 6, 6101u, 0.10f, 0.38f);
    glPopMatrix();

    // Roots flaring out over that mound and into the ground.
    glColor3f(0.28f, 0.20f, 0.13f);
    for (int i = 0; i < 7; ++i) {
        float angle = (static_cast<float>(i) / 7.0f) * 360.0f;
        glPushMatrix();
        glRotatef(angle, 0.0f, 1.0f, 0.0f);
        glTranslatef(0.50f, 0.35f, 0.0f);
        glRotatef(72.0f, 0.0f, 0.0f, 1.0f);
        prim::drawCylinder(0.30f, 0.09f, 1.9f, 6, false, false);
        glPopMatrix();
    }
    glColor3f(0.30f, 0.21f, 0.14f);

    // Canopy above the house.
    glColor3f(0.16f, 0.33f, 0.16f);
    glPushMatrix();
    glTranslatef(0.0f, 10.2f, 0.0f);
    prim::drawBlob(2.9f, 11, 8, 4201u, 0.18f, 0.80f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(1.7f, 9.1f, 1.1f);
    prim::drawBlob(1.7f, 9, 6, 4211u, 0.20f, 0.85f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-1.6f, 9.3f, -1.2f);
    prim::drawBlob(1.6f, 9, 6, 4231u, 0.20f, 0.85f);
    glPopMatrix();

    // --- support struts ----------------------------------------------------
    glColor3f(WOOD_DARK.x, WOOD_DARK.y, WOOD_DARK.z);
    const float strutX[4] = { -1.7f, 1.7f, -1.7f, 1.7f };
    const float strutZ[4] = { -1.4f, -1.4f, 1.4f, 1.4f };
    for (int i = 0; i < 4; ++i) {
        glPushMatrix();
        glTranslatef(strutX[i] * 0.55f, -BURY, strutZ[i] * 0.55f);
        // Lean each strut outward to meet the platform corner.
        float leanX = strutX[i] * 0.45f / platformY;
        float leanZ = strutZ[i] * 0.45f / platformY;
        glRotatef(util::toDegrees(std::atan(leanX)), 0.0f, 0.0f, -1.0f);
        glRotatef(util::toDegrees(std::atan(leanZ)), 1.0f, 0.0f, 0.0f);
        prim::drawCylinder(0.16f, 0.13f, platformY + BURY, 6, false, false);
        glPopMatrix();
    }

    // --- platform ----------------------------------------------------------
    // Four stout branches visually tie the deck into the host tree. The
    // platform remains a real solid surface, but no longer reads like a
    // freestanding cabin placed beside a trunk.
    glColor3f(WOOD_DARK.x, WOOD_DARK.y, WOOD_DARK.z);
    for (int i = 0; i < 4; ++i) {
        float angle = 45.0f + static_cast<float>(i) * 90.0f;
        glPushMatrix();
        glRotatef(angle, 0.0f, 1.0f, 0.0f);
        glTranslatef(1.35f, platformY - 0.10f, 0.0f);
        glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
        prim::drawCylinder(0.15f, 0.10f, 2.1f, 7, false, false);
        glPopMatrix();
    }

    glColor3f(WOOD_MID.x, WOOD_MID.y, WOOD_MID.z);
    glPushMatrix();
    glTranslatef(0.0f, platformY + 0.12f, 0.0f);
    prim::drawBox(platformW, 0.24f, platformD);
    glPopMatrix();

    // Plank lines on the deck, for a bit of texture without a texture.
    glColor3f(WOOD_LIGHT.x, WOOD_LIGHT.y, WOOD_LIGHT.z);
    for (int i = 0; i < 7; ++i) {
        float x = -platformW * 0.5f + 0.35f + static_cast<float>(i) * 0.6f;
        glPushMatrix();
        glTranslatef(x, platformY + 0.25f, 0.0f);
        prim::drawBox(0.5f, 0.03f, platformD - 0.15f);
        glPopMatrix();
    }

    // --- walls -------------------------------------------------------------
    const float wallH = 2.1f;
    const float roomW = 3.0f;
    const float roomD = 2.6f;
    const float wallY = platformY + 0.24f;

    glColor3f(WOOD_LIGHT.x, WOOD_LIGHT.y, WOOD_LIGHT.z);

    // back wall
    glPushMatrix();
    glTranslatef(0.0f, wallY + wallH * 0.5f, -roomD * 0.5f);
    prim::drawBox(roomW, wallH, 0.14f);
    glPopMatrix();

    // left wall
    glPushMatrix();
    glTranslatef(-roomW * 0.5f, wallY + wallH * 0.5f, 0.0f);
    prim::drawBox(0.14f, wallH, roomD);
    glPopMatrix();

    // right wall
    glPushMatrix();
    glTranslatef(roomW * 0.5f, wallY + wallH * 0.5f, 0.0f);
    prim::drawBox(0.14f, wallH, roomD);
    glPopMatrix();

    // front wall, split either side of a doorway
    float doorWidth = 0.85f;
    float sideWidth = (roomW - doorWidth) * 0.5f;
    for (int side = 0; side < 2; ++side) {
        float sign = side == 0 ? -1.0f : 1.0f;
        glPushMatrix();
        glTranslatef(sign * (doorWidth * 0.5f + sideWidth * 0.5f),
                     wallY + wallH * 0.5f, roomD * 0.5f);
        prim::drawBox(sideWidth, wallH, 0.14f);
        glPopMatrix();
    }
    // lintel above the door
    glPushMatrix();
    glTranslatef(0.0f, wallY + wallH - 0.2f, roomD * 0.5f);
    prim::drawBox(doorWidth, 0.4f, 0.14f);
    glPopMatrix();

    // --- door --------------------------------------------------------------
    glColor3f(WOOD_DARK.x, WOOD_DARK.y, WOOD_DARK.z);
    glPushMatrix();
    glTranslatef(-doorWidth * 0.42f, wallY + 0.85f, roomD * 0.5f + 0.08f);
    glRotatef(-38.0f, 0.0f, 1.0f, 0.0f);   // ajar
    glTranslatef(doorWidth * 0.42f, 0.0f, 0.0f);
    prim::drawBox(doorWidth * 0.92f, 1.7f, 0.06f);
    glPopMatrix();

    // --- roof --------------------------------------------------------------
    glColor3f(WOOD_ROOF.x, WOOD_ROOF.y, WOOD_ROOF.z);
    glPushMatrix();
    glTranslatef(0.0f, wallY + wallH, 0.0f);
    prim::drawPyramid(roomW + 0.9f, roomD + 0.9f, 1.35f);
    glPopMatrix();

    // ridge beam
    glColor3f(WOOD_DARK.x, WOOD_DARK.y, WOOD_DARK.z);
    glPushMatrix();
    glTranslatef(0.0f, wallY + wallH + 1.3f, 0.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    prim::drawCylinder(0.07f, 0.07f, roomD + 1.1f, 5, false, false);
    glPopMatrix();

    // --- balcony railing ---------------------------------------------------
    glColor3f(WOOD_MID.x, WOOD_MID.y, WOOD_MID.z);
    const float railY = platformY + 0.24f;
    const float balconyZ = platformD * 0.5f - 0.1f;

    for (int i = 0; i < 8; ++i) {
        float x = -platformW * 0.5f + 0.25f
                + static_cast<float>(i) * ((platformW - 0.5f) / 7.0f);
        glPushMatrix();
        glTranslatef(x, railY, balconyZ);
        prim::drawCylinder(0.05f, 0.045f, 0.75f, 5, false, false);
        glPopMatrix();
    }
    // top rail
    glPushMatrix();
    glTranslatef(0.0f, railY + 0.78f, balconyZ);
    prim::drawBox(platformW - 0.3f, 0.08f, 0.08f);
    glPopMatrix();

    // side railings
    for (int side = 0; side < 2; ++side) {
        float sign = side == 0 ? -1.0f : 1.0f;
        for (int i = 0; i < 4; ++i) {
            float z = balconyZ - 0.35f - static_cast<float>(i) * 0.5f;
            glPushMatrix();
            glTranslatef(sign * (platformW * 0.5f - 0.2f), railY, z);
            prim::drawCylinder(0.05f, 0.045f, 0.75f, 5, false, false);
            glPopMatrix();
        }
        glPushMatrix();
        glTranslatef(sign * (platformW * 0.5f - 0.2f), railY + 0.78f, balconyZ - 1.1f);
        prim::drawBox(0.08f, 0.08f, 1.7f);
        glPopMatrix();
    }

    // --- ladder ------------------------------------------------------------
    glColor3f(WOOD_DARK.x, WOOD_DARK.y, WOOD_DARK.z);
    const float ladderZ = platformD * 0.5f + 0.35f;
    for (int side = 0; side < 2; ++side) {
        float sign = side == 0 ? -1.0f : 1.0f;
        glPushMatrix();
        glTranslatef(sign * 0.38f, -1.2f, ladderZ);
        glRotatef(-9.0f, 1.0f, 0.0f, 0.0f);
        prim::drawCylinder(0.07f, 0.06f, platformY + 1.5f, 5, false, false);
        glPopMatrix();
    }
    int rungs = static_cast<int>(platformY / 0.42f);
    for (int i = 1; i <= rungs; ++i) {
        float y = static_cast<float>(i) * 0.42f;
        glPushMatrix();
        glTranslatef(0.0f, y, ladderZ + (y / platformY) * 0.15f);
        glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
        prim::drawCylinder(0.045f, 0.045f, 0.76f, 4, false, false);
        glPopMatrix();
    }

    // --- lantern housing on the balcony ------------------------------------
    glColor3f(0.25f, 0.20f, 0.15f);
    glPushMatrix();
    glTranslatef(1.65f, railY + 0.78f, 1.45f);
    prim::drawCylinder(0.06f, 0.05f, 0.34f, 5, false, false);
    glTranslatef(0.0f, 0.34f, 0.0f);
    prim::drawBox(0.26f, 0.05f, 0.26f);
    glPopMatrix();
}

void Environment::buildProps() {
    // --- bench -------------------------------------------------------------
    m_benchList = glGenLists(1);
    if (m_benchList) {
        glNewList(m_benchList, GL_COMPILE);
        // seat
        glPushMatrix();
        glTranslatef(0.0f, 0.42f, 0.0f);
        prim::drawBox(1.5f, 0.12f, 0.42f);
        glPopMatrix();
        // legs
        for (int i = 0; i < 4; ++i) {
            float x = (i % 2 == 0) ? -0.6f : 0.6f;
            float z = (i < 2) ? -0.15f : 0.15f;
            glPushMatrix();
            glTranslatef(x, 0.0f, z);
            prim::drawCylinder(0.06f, 0.055f, 0.42f, 5, false, false);
            glPopMatrix();
        }
        glEndList();
    }

    // --- log seat ----------------------------------------------------------
    m_logSeatList = glGenLists(1);
    if (m_logSeatList) {
        glNewList(m_logSeatList, GL_COMPILE);
        glPushMatrix();
        glTranslatef(-0.75f, 0.28f, 0.0f);
        glRotatef(90.0f, 0.0f, 0.0f, -1.0f);
        prim::drawCylinder(0.28f, 0.26f, 1.5f, 8, true, true);
        glPopMatrix();
        glEndList();
    }
}

void Environment::buildRenderData() {
    if (m_built) return;

    m_treehouseList = glGenLists(1);
    if (m_treehouseList) {
        glNewList(m_treehouseList, GL_COMPILE);
        buildTreehouse();
        glEndList();
    }

    buildProps();

    m_built = true;
    util::logInfo("Treehouse and props compiled into display lists.");
}

void Environment::destroyRenderData() {
    if (!m_built) return;
    if (m_treehouseList) glDeleteLists(m_treehouseList, 1);
    if (m_benchList)     glDeleteLists(m_benchList, 1);
    if (m_logSeatList)   glDeleteLists(m_logSeatList, 1);
    m_treehouseList = m_benchList = m_logSeatList = 0;
    m_built = false;
}

void Environment::update(float deltaTime, const RenderContext& context) {
    m_time += deltaTime;
    (void)context;
    // A slow, shallow flicker - an oil lamp, not a bonfire.
    m_lanternFlicker = 0.88f + util::valueNoise2D(m_time * 1.35f, 4.0f, m_seed + 17u) * 0.24f;
}

void Environment::applyLanternLight(GLenum light, float nightFactor) const {
    const GLfloat position[] = { m_lanternPosition.x, m_lanternPosition.y,
                                 m_lanternPosition.z, 1.0f };

    float strength = m_lanternFlicker * nightFactor * 1.15f;

    const GLfloat diffuse[] = { 1.00f * strength, 0.80f * strength, 0.45f * strength, 1.0f };
    const GLfloat ambient[] = { 0.08f * strength, 0.06f * strength, 0.03f * strength, 1.0f };

    glLightfv(light, GL_POSITION, position);
    glLightfv(light, GL_DIFFUSE,  diffuse);
    glLightfv(light, GL_AMBIENT,  ambient);
    glLightf(light, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf(light, GL_LINEAR_ATTENUATION,    0.09f);
    glLightf(light, GL_QUADRATIC_ATTENUATION, 0.022f);

    // No point paying for the light in broad daylight.
    if (nightFactor > 0.03f) glEnable(light);
    else                     glDisable(light);
}

void Environment::renderSolid(const RenderContext& context) const {
    if (!m_built) return;

    // --- treehouse ---------------------------------------------------------
    Vec3 centre = m_treehousePosition + Vec3(0.0f, 6.0f, 0.0f);
    if (context.frustum.sphereVisible(centre, 8.5f)
        && util::distanceXZ(m_treehousePosition, context.cameraPosition) < context.drawDistance) {

        ++context.visibleProps;
        glPushMatrix();
        glTranslatef(m_treehousePosition.x, m_treehousePosition.y, m_treehousePosition.z);
        glRotatef(-24.0f, 0.0f, 1.0f, 0.0f);
        if (m_treehouseList) glCallList(m_treehouseList);
        glPopMatrix();
    }

    // --- props -------------------------------------------------------------
    for (size_t i = 0; i < m_props.size(); ++i) {
        const Prop& prop = m_props[i];

        if (!context.frustum.sphereVisible(prop.position + Vec3(0.0f, 0.8f, 0.0f), 2.2f))
            continue;
        if (util::distanceXZ(prop.position, context.cameraPosition) > context.drawDistance)
            continue;

        ++context.visibleProps;

        glPushMatrix();
        glTranslatef(prop.position.x, prop.position.y, prop.position.z);
        glRotatef(prop.rotation, 0.0f, 1.0f, 0.0f);
        glScalef(prop.scale, prop.scale, prop.scale);
        glColor3f(prop.color.x, prop.color.y, prop.color.z);

        if (prop.type == 0 && m_benchList) {
            glCallList(m_benchList);
        } else if (prop.type == 1 && m_logSeatList) {
            glCallList(m_logSeatList);
        } else if (prop.type == 2) {
            // lantern post
            prim::drawCylinder(0.09f, 0.07f, 2.1f, 6, false, false);
            glPushMatrix();
            glTranslatef(0.0f, 2.1f, 0.0f);
            glColor3f(0.22f, 0.18f, 0.14f);
            prim::drawBox(0.24f, 0.06f, 0.24f);
            glPopMatrix();
        }
        glPopMatrix();
    }
}

// Windows and lantern glass. Drawn unlit so they hold their colour regardless
// of how dark the scene has become - which is the whole point of a light
// source you can see from across the island.
void Environment::renderEmissive(const RenderContext& context) const {
    if (!m_built) return;

    float night = context.nightFactor;
    float lampGlow = night * m_lanternFlicker;

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT
                | GL_COLOR_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();
    glTranslatef(m_treehousePosition.x, m_treehousePosition.y, m_treehousePosition.z);
    glRotatef(-24.0f, 0.0f, 1.0f, 0.0f);

    const float wallY = m_platformHeight + 0.24f;
    const float roomW = 3.0f;
    const float roomD = 2.6f;

    // Window glass: dark and reflective by day, warmly lit at night.
    Vec3 dayGlass(0.16f, 0.20f, 0.24f);
    Vec3 nightGlass(1.00f, 0.78f, 0.38f);
    Vec3 glass = util::lerpv(dayGlass, nightGlass, lampGlow);

    glColor4f(glass.x, glass.y, glass.z, 1.0f);

    // two side windows and one on the back wall
    glPushMatrix();
    glTranslatef(-roomW * 0.5f - 0.02f, wallY + 1.25f, 0.35f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    prim::drawBox(0.9f, 0.7f, 0.04f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(roomW * 0.5f + 0.02f, wallY + 1.25f, -0.25f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    prim::drawBox(0.9f, 0.7f, 0.04f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.35f, wallY + 1.25f, -roomD * 0.5f - 0.02f);
    prim::drawBox(0.85f, 0.7f, 0.04f);
    glPopMatrix();

    // Window frames
    glColor4f(WOOD_DARK.x, WOOD_DARK.y, WOOD_DARK.z, 1.0f);
    glPushMatrix();
    glTranslatef(-roomW * 0.5f - 0.03f, wallY + 1.25f, 0.35f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    prim::drawBox(0.08f, 0.72f, 0.05f);
    glPopMatrix();

    // Lantern glass on the balcony
    Vec3 lampColor = util::lerpv(Vec3(0.30f, 0.28f, 0.24f),
                                 Vec3(1.00f, 0.86f, 0.52f), lampGlow);
    glColor4f(lampColor.x, lampColor.y, lampColor.z, 1.0f);
    glPushMatrix();
    glTranslatef(1.65f, m_platformHeight + 0.24f + 0.78f + 0.17f, 1.45f);
    prim::drawBox(0.17f, 0.24f, 0.17f);
    glPopMatrix();

    glPopMatrix();

    // --- glowing halo around the lantern at night --------------------------
    if (lampGlow > 0.05f) {
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glPushMatrix();
        glTranslatef(m_lanternPosition.x, m_lanternPosition.y, m_lanternPosition.z);

        // Camera-facing disc, built from the inverted view rotation.
        GLfloat modelview[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
        modelview[0] = 1.0f; modelview[1] = 0.0f; modelview[2]  = 0.0f;
        modelview[4] = 0.0f; modelview[5] = 1.0f; modelview[6]  = 0.0f;
        modelview[8] = 0.0f; modelview[9] = 0.0f; modelview[10] = 1.0f;
        glLoadMatrixf(modelview);

        glBegin(GL_TRIANGLE_FAN);
            glColor4f(1.0f, 0.82f, 0.45f, 0.42f * lampGlow);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor4f(1.0f, 0.70f, 0.30f, 0.0f);
            for (int i = 0; i <= 14; ++i) {
                float angle = (static_cast<float>(i) / 14.0f) * util::TWO_PI;
                glVertex3f(std::cos(angle) * 0.85f, std::sin(angle) * 0.85f, 0.0f);
            }
        glEnd();
        glPopMatrix();
        glDepthMask(GL_TRUE);
    }

    // Lantern posts along the path also light up.
    if (lampGlow > 0.05f) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        Vec3 postGlass = util::lerpv(Vec3(0.28f, 0.26f, 0.22f),
                                     Vec3(1.00f, 0.84f, 0.46f), lampGlow);
        glColor4f(postGlass.x, postGlass.y, postGlass.z, 1.0f);

        for (size_t i = 0; i < m_props.size(); ++i) {
            const Prop& prop = m_props[i];
            if (prop.type != 2) continue;
            if (util::distanceXZ(prop.position, context.cameraPosition) > 120.0f) continue;

            glPushMatrix();
            glTranslatef(prop.position.x, prop.position.y + 1.95f, prop.position.z);
            prim::drawBox(0.15f, 0.22f, 0.15f);
            glPopMatrix();
        }
    }

    glPopAttrib();
}
