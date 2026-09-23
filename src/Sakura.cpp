// ============================================================================
// Sakura.cpp - procedural hero cherry-blossom tree.
// ============================================================================
#include "Sakura.h"
#include "Config.h"
#include "Primitives.h"
#include "Terrain.h"

using util::Vec3;

namespace {

void branchSegment(float yaw, float tilt, float length,
                   float baseRadius, float tipRadius)
{
    glPushMatrix();
    glRotatef(yaw, 0.0f, 1.0f, 0.0f);
    glRotatef(tilt, 0.0f, 0.0f, 1.0f);
    prim::drawCylinder(baseRadius, tipRadius, length, 8, true, false);
    glPopMatrix();
}

void blossomCluster(const Vec3& p, float radius, const Vec3& color, unsigned seed)
{
    glPushMatrix();
    glTranslatef(p.x, p.y, p.z);
    glColor3f(color.x, color.y, color.z);
    prim::drawBlob(radius, 7, 5, seed, 0.22f, 0.82f);

    // A few smaller overlapping masses break up the silhouette and produce
    // the layered, fluffy look of real blossom clusters.
    glColor3f(util::clampf(color.x + 0.07f, 0.0f, 1.0f),
              util::clampf(color.y + 0.04f, 0.0f, 1.0f),
              util::clampf(color.z + 0.06f, 0.0f, 1.0f));
    glPushMatrix();
    glTranslatef(radius * 0.48f, radius * 0.08f, radius * 0.18f);
    prim::drawBlob(radius * 0.60f, 6, 4, seed + 17u, 0.20f, 0.88f);
    glPopMatrix();

    glColor3f(util::clampf(color.x - 0.04f, 0.0f, 1.0f),
              util::clampf(color.y - 0.02f, 0.0f, 1.0f),
              util::clampf(color.z - 0.01f, 0.0f, 1.0f));
    glPushMatrix();
    glTranslatef(-radius * 0.38f, radius * 0.12f, -radius * 0.20f);
    prim::drawBlob(radius * 0.52f, 6, 4, seed + 31u, 0.20f, 0.88f);
    glPopMatrix();
    glPopMatrix();
}

} // namespace

SakuraTree::SakuraTree()
    : m_position(0.0f, 0.0f, 0.0f)
    , m_seed(1u)
    , m_time(0.0f)
    , m_sway(0.0f)
    , m_swingFacing(90.0f)
    , m_renderList(0)
    , m_built(false)
{
}

void SakuraTree::initialize(const Terrain& terrain, float x, float z, unsigned seed) {
    m_seed = seed;
    m_position = Vec3(x, terrain.heightAt(x, z), z);
    m_time = 0.0f;
    m_sway = 0.0f;
    // The hero branch extends toward +X, i.e. toward the campfire area.
    m_swingFacing = 90.0f;
}

void SakuraTree::buildRenderData() {
    if (m_built) return;

    m_renderList = glGenLists(1);
    if (!m_renderList) {
        util::logWarning("Could not allocate Sakura display list.");
        return;
    }

    glNewList(m_renderList, GL_COMPILE);

    // --- thick, naturally segmented trunk ---------------------------------
    glColor3f(0.25f, 0.13f, 0.09f);
    prim::drawCylinder(0.72f, 0.56f, 2.8f, 11, true, false);

    glPushMatrix();
    glTranslatef(0.0f, 2.65f, 0.0f);
    glRotatef(-7.0f, 0.0f, 0.0f, 1.0f);
    prim::drawCylinder(0.57f, 0.39f, 2.25f, 10, false, false);
    glTranslatef(0.0f, 2.15f, 0.0f);
    glRotatef(9.0f, 0.0f, 0.0f, 1.0f);
    prim::drawCylinder(0.40f, 0.23f, 2.05f, 9, false, false);
    glPopMatrix();

    // Root flares.
    for (int i = 0; i < 7; ++i) {
        float yaw = static_cast<float>(i) * 360.0f / 7.0f;
        glPushMatrix();
        glRotatef(yaw, 0.0f, 1.0f, 0.0f);
        glTranslatef(0.34f, 0.12f, 0.0f);
        glRotatef(74.0f, 0.0f, 0.0f, -1.0f);
        prim::drawCylinder(0.18f, 0.035f, 0.78f, 7, false, false);
        glPopMatrix();
    }

    // --- major spreading branches ----------------------------------------
    // Branch A is the strong horizontal swing branch. It now extends toward
    // the LEFT side of the trunk so the swing reads as a natural side limb.
    glColor3f(0.27f, 0.14f, 0.095f);
    glPushMatrix();
    glTranslatef(0.0f, 4.72f, 0.0f);
    glRotatef(88.0f, 0.0f, 0.0f, 1.0f);
    prim::drawCylinder(0.27f, 0.12f, 4.25f, 9, false, false);
    glTranslatef(0.0f, 4.05f, 0.0f);
    glRotatef(8.0f, 0.0f, 0.0f, 1.0f);
    prim::drawCylinder(0.13f, 0.045f, 1.65f, 7, false, false);
    glPopMatrix();

    // Branch B - broad left sweep.
    glPushMatrix();
    glTranslatef(-0.05f, 4.45f, 0.0f);
    branchSegment(250.0f, 62.0f, 3.35f, 0.25f, 0.095f);
    glTranslatef(0.0f, 3.05f, 0.0f);
    branchSegment(18.0f, 54.0f, 1.65f, 0.11f, 0.035f);
    glPopMatrix();

    // Small left-side fork branches make the silhouette less symmetrical
    // and give the new swing limb a more believable flowering continuation.
    glPushMatrix();
    glTranslatef(-0.20f, 4.95f, 0.12f);
    branchSegment(228.0f, 48.0f, 2.35f, 0.15f, 0.045f);
    glTranslatef(0.0f, 2.15f, 0.0f);
    branchSegment(12.0f, 43.0f, 1.35f, 0.065f, 0.018f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.12f, 5.65f, 0.18f);
    branchSegment(205.0f, 42.0f, 2.05f, 0.13f, 0.038f);
    glTranslatef(0.0f, 1.85f, 0.0f);
    branchSegment(335.0f, 40.0f, 1.20f, 0.055f, 0.016f);
    glPopMatrix();

    // Branch C - rear high canopy.
    glPushMatrix();
    glTranslatef(0.05f, 5.55f, -0.05f);
    branchSegment(158.0f, 55.0f, 3.25f, 0.22f, 0.08f);
    glTranslatef(0.0f, 2.95f, 0.0f);
    branchSegment(335.0f, 48.0f, 1.45f, 0.095f, 0.03f);
    glPopMatrix();

    // Branch D - right/rear umbrella.
    glPushMatrix();
    glTranslatef(0.0f, 5.05f, 0.0f);
    branchSegment(42.0f, 56.0f, 3.1f, 0.20f, 0.075f);
    glTranslatef(0.0f, 2.75f, 0.0f);
    branchSegment(308.0f, 45.0f, 1.65f, 0.09f, 0.028f);
    glPopMatrix();

    // --- many thin twigs --------------------------------------------------
    const float twigYaws[] = { 12.0f, 52.0f, 88.0f, 128.0f, 176.0f, 218.0f,
                               264.0f, 305.0f, 344.0f };
    for (int i = 0; i < 9; ++i) {
        float y = 5.5f + static_cast<float>(i % 3) * 0.55f;
        glPushMatrix();
        glTranslatef(std::cos(util::toRadians(twigYaws[i])) * 0.28f,
                     y,
                     std::sin(util::toRadians(twigYaws[i])) * 0.24f);
        branchSegment(twigYaws[i], 62.0f + static_cast<float>(i % 2) * 7.0f,
                      1.65f + static_cast<float>(i % 3) * 0.35f,
                      0.065f, 0.018f);
        glPopMatrix();
    }

    // --- hero ground dressing --------------------------------------------
    // A small local patch guarantees that the Sakura has the same intimate
    // grass-and-flower detail as the reference even though the global
    // vegetation scatterer keeps a safe radius around the trunk.
    util::Rng groundRng(m_seed ^ 0x71A55A11u);
    for (int i = 0; i < 15; ++i) {
        float a = groundRng.range(0.0f, util::TWO_PI);
        float r = groundRng.range(0.9f, 2.35f);
        glColor3f(0.20f + groundRng.nextFloat() * 0.12f,
                  0.42f + groundRng.nextFloat() * 0.18f,
                  0.12f + groundRng.nextFloat() * 0.10f);
        glPushMatrix();
        glTranslatef(std::cos(a) * r, 0.02f, std::sin(a) * r);
        glRotatef(groundRng.range(0.0f, 360.0f), 0.0f, 1.0f, 0.0f);
        prim::drawCrossQuads(0.10f, groundRng.range(0.20f, 0.34f));
        glPopMatrix();
    }

    const Vec3 groundFlowers[4] = {
        Vec3(0.98f, 0.62f, 0.76f), Vec3(1.0f, 0.84f, 0.91f),
        Vec3(0.95f, 0.93f, 0.55f), Vec3(0.82f, 0.55f, 0.90f)
    };
    for (int i = 0; i < 10; ++i) {
        float a = groundRng.range(0.0f, util::TWO_PI);
        float r = groundRng.range(1.0f, 2.55f);
        float y = 0.17f;
        Vec3 c = groundFlowers[groundRng.rangeInt(0, 3)];
        glPushMatrix();
        glTranslatef(std::cos(a) * r, y, std::sin(a) * r);
        glColor3f(0.18f, 0.46f, 0.16f);
        prim::drawCylinder(0.012f, 0.009f, 0.18f, 5, false, false);
        glTranslatef(0.0f, 0.16f, 0.0f);
        glColor3f(c.x, c.y, c.z);
        prim::drawBlob(0.075f, 6, 4, 1200u + static_cast<unsigned>(i), 0.16f, 0.72f);
        glPopMatrix();
    }

    // A soft carpet of fallen Sakura petals.  These static accents are kept lightweight
    // static accents; the rolling particle pool supplies the continuous
    // falling petals overhead.
    for (int i = 0; i < 64; ++i) {
        float a = groundRng.range(0.0f, util::TWO_PI);
        float r = std::sqrt(groundRng.nextFloat()) * 3.45f;
        // A small windward stretch makes the carpet flow naturally away from
        // the trunk while remaining concentrated beneath the canopy.
        float windStretch = 0.35f + groundRng.nextFloat() * 0.45f;
        float px = std::cos(a) * r + windStretch * 0.55f;
        float pz = std::sin(a) * r + windStretch * 0.12f;
        float y = 0.045f + groundRng.range(0.0f, 0.018f);
        glPushMatrix();
        glTranslatef(px, y, pz);
        glRotatef(groundRng.range(0.0f, 360.0f), 0.0f, 1.0f, 0.0f);
        glScalef(1.0f, 0.16f, 0.72f);
        glColor3f(0.90f + groundRng.nextFloat() * 0.08f,
                  0.42f + groundRng.nextFloat() * 0.20f,
                  0.60f + groundRng.nextFloat() * 0.22f);
        prim::drawBlob(0.09f, 5, 3, 1700u + static_cast<unsigned>(i), 0.10f, 0.75f);
        glPopMatrix();
    }

    // --- dense blossom canopy --------------------------------------------
    // Deliberately irregular positions follow the branch directions instead
    // of forming a single sphere. Palette variation keeps the blossoms from
    // becoming one flat pink texture.
    util::Rng rng(m_seed ^ 0x5A6B2A11u);
    const Vec3 palette[6] = {
        Vec3(0.96f, 0.48f, 0.67f),
        Vec3(1.00f, 0.61f, 0.76f),
        Vec3(0.93f, 0.39f, 0.58f),
        Vec3(1.00f, 0.70f, 0.82f),
        Vec3(0.88f, 0.31f, 0.50f),
        Vec3(0.98f, 0.55f, 0.72f)
    };

    for (int i = 0; i < 56; ++i) {
        float a = rng.range(0.0f, util::TWO_PI);
        float ring = rng.range(1.25f, 3.9f);
        float x = std::cos(a) * ring * 1.12f;
        float z = std::sin(a) * ring * 0.82f;
        float y = rng.range(4.55f, 7.75f) + (1.0f - ring / 4.0f) * 0.85f;

        // Bias several clusters along the strong LEFT horizontal branch so
        // the swing sits beneath a visibly flowering limb rather than under
        // an empty branch.
        if (i < 17) {
            x = rng.range(-4.45f, -1.15f);
            z = rng.range(-0.65f, 0.80f);
            y = rng.range(5.25f, 6.45f);
        }

        float radius = rng.range(0.34f, 0.62f);
        blossomCluster(Vec3(x, y, z), radius,
                       palette[rng.rangeInt(0, 5)], 900u + static_cast<unsigned>(i) * 13u);
    }

    glEndList();
    m_built = true;
    util::logInfo("Sakura hero tree geometry compiled.");
}

void SakuraTree::destroyRenderData() {
    if (m_renderList) glDeleteLists(m_renderList, 1);
    m_renderList = 0;
    m_built = false;
}

void SakuraTree::update(float deltaTime, const RenderContext& context) {
    m_time += deltaTime;
    float wind = 0.35f + context.wind * 0.85f;
    m_sway = std::sin(m_time * (0.75f + context.wind * 0.4f)) * wind * 1.8f;
}

void SakuraTree::render(const RenderContext& context) const {
    if (!m_built) return;
    if (!context.frustum.sphereVisible(m_position + Vec3(0.0f, 4.5f, 0.0f), 6.6f)) return;

    ++context.visibleProps;
    glPushMatrix();
    glTranslatef(m_position.x, m_position.y, m_position.z);
    glRotatef(m_sway, 0.0f, 0.0f, 1.0f);
    glCallList(m_renderList);
    glPopMatrix();
}

Vec3 SakuraTree::swingAnchor() const {
    // World-space point on the strong LEFT branch. Keep this independent from
    // the visual sway so the swing remains a stable, believable interaction.
    // The branch is visually horizontal at about y=4.8 in the local tree.
    // Put the rope anchors directly on that branch instead of above it.
    return m_position + Vec3(-3.65f, 4.82f, 0.0f);
}
