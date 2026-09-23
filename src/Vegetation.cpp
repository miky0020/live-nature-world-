#include "Vegetation.h"
#include "Config.h"
#include "Primitives.h"
#include "Terrain.h"

#include <cstdio>

using util::Vec3;

Vegetation::Vegetation()
    : m_gridRes(0)
    , m_gridCellSize(16.0f)
    , m_gridOrigin(0.0f)
    , m_grassList(0)
    , m_grassMidList(0)
    , m_grassFarList(0)
    , m_flowerList(0)
    , m_listsBuilt(false)
    , m_grassDrawDistance(82.0f)
    , m_flowerDrawDistance(46.0f)
    , m_grassNearDistance(20.0f)
    , m_grassMidDistance(46.0f)
{
    for (int i = 0; i < ROCK_VARIANT_COUNT; ++i) m_rockLists[i] = 0;
}

Vegetation::~Vegetation() {}

// ---------------------------------------------------------------------------
//  Placement
// ---------------------------------------------------------------------------
void Vegetation::generate(const Terrain& terrain, unsigned seed,
                          const std::vector<util::Zone>& exclusions)
{
    m_grass.clear();
    m_flowers.clear();
    m_rocks.clear();

    util::Rng rng(seed ^ 0x51A55u);
    const float half = terrain.halfSize();

    // --- grass -------------------------------------------------------------
    m_grass.reserve(static_cast<size_t>(Config::GRASS_CLUSTER_COUNT));
    int attempts = 0;
    while (static_cast<int>(m_grass.size()) < Config::GRASS_CLUSTER_COUNT
           && attempts < Config::GRASS_CLUSTER_COUNT * 12) {
        ++attempts;

        float x = rng.range(-half + 4.0f, half - 4.0f);
        float z = rng.range(-half + 4.0f, half - 4.0f);
        float ground = terrain.heightAt(x, z);

        if (ground < Config::WATER_LEVEL + 1.0f) continue;   // not in the sea
        if (terrain.normalAt(x, z).y < 0.74f) continue;      // not on cliffs
        if (Terrain::pathInfluence(x, z) > 0.42f) continue;  // not on the paths

        // Grass may grow near the landmarks but not through them.
        bool blocked = false;
        for (size_t i = 0; i < exclusions.size(); ++i) {
            if (exclusions[i].contains(x, z)) { blocked = true; break; }
        }
        if (blocked) continue;

        GrassInstance g;
        g.position = Vec3(x, ground, z);
        g.scale    = rng.range(0.70f, 1.25f);
        g.rotation = rng.range(0.0f, 180.0f);
        g.phase    = rng.range(0.0f, util::TWO_PI);

        float shade = rng.nextFloat();
        g.color = util::lerpv(Vec3(0.22f, 0.42f, 0.16f),
                              Vec3(0.52f, 0.68f, 0.30f), shade);
        m_grass.push_back(g);
    }

    // --- flowers -----------------------------------------------------------
    // Clustered rather than uniform: flowers grow in patches.
    const Vec3 flowerPalette[5] = {
        Vec3(0.92f, 0.86f, 0.35f),   // yellow
        Vec3(0.88f, 0.42f, 0.52f),   // pink
        Vec3(0.94f, 0.94f, 0.92f),   // white
        Vec3(0.62f, 0.48f, 0.86f),   // violet
        Vec3(0.94f, 0.58f, 0.28f)    // orange
    };

    m_flowers.reserve(static_cast<size_t>(Config::FLOWER_COUNT));
    int patches = Config::FLOWER_COUNT / 12 + 1;
    for (int p = 0; p < patches && static_cast<int>(m_flowers.size()) < Config::FLOWER_COUNT; ++p) {
        float cx = rng.range(-half + 10.0f, half - 10.0f);
        float cz = rng.range(-half + 10.0f, half - 10.0f);
        Vec3 patchColor = flowerPalette[rng.rangeInt(0, 4)];

        int perPatch = rng.rangeInt(6, 14);
        for (int i = 0; i < perPatch && static_cast<int>(m_flowers.size()) < Config::FLOWER_COUNT; ++i) {
            float x = cx + rng.range(-3.5f, 3.5f);
            float z = cz + rng.range(-3.5f, 3.5f);
            float ground = terrain.heightAt(x, z);

            if (ground < Config::WATER_LEVEL + 1.4f) continue;
            if (terrain.normalAt(x, z).y < 0.82f) continue;
            if (Terrain::pathInfluence(x, z) > 0.2f) continue;

            bool blocked = false;
            for (size_t e = 0; e < exclusions.size(); ++e) {
                if (exclusions[e].contains(x, z)) { blocked = true; break; }
            }
            if (blocked) continue;

            FlowerInstance f;
            f.position = Vec3(x, ground, z);
            f.scale    = rng.range(0.80f, 1.25f);
            f.rotation = rng.range(0.0f, 360.0f);
            f.phase    = rng.range(0.0f, util::TWO_PI);
            f.color    = util::lerpv(patchColor, Vec3(1.0f, 1.0f, 1.0f),
                                     rng.range(0.0f, 0.22f));
            m_flowers.push_back(f);
        }
    }

    // --- rocks -------------------------------------------------------------
    m_rocks.reserve(static_cast<size_t>(Config::ROCK_COUNT));
    attempts = 0;
    while (static_cast<int>(m_rocks.size()) < Config::ROCK_COUNT
           && attempts < Config::ROCK_COUNT * 40) {
        ++attempts;

        float x = rng.range(-half + 6.0f, half - 6.0f);
        float z = rng.range(-half + 6.0f, half - 6.0f);
        float ground = terrain.heightAt(x, z);

        if (ground < Config::WATER_LEVEL - 0.5f) continue;
        if (Terrain::pathInfluence(x, z) > 0.1f) continue;   // never block a path

        bool blocked = false;
        for (size_t i = 0; i < exclusions.size(); ++i) {
            if (exclusions[i].contains(x, z)) { blocked = true; break; }
        }
        if (blocked) continue;

        RockInstance r;
        r.position = Vec3(x, ground - 0.15f, z);   // settle slightly into the ground
        r.scale    = rng.range(0.35f, 1.6f);
        r.rotation = rng.range(0.0f, 360.0f);
        r.tilt     = rng.range(-14.0f, 14.0f);
        r.variant  = rng.rangeInt(0, ROCK_VARIANT_COUNT - 1);

        float shade = rng.nextFloat();
        r.color = util::lerpv(Vec3(0.34f, 0.33f, 0.31f),
                              Vec3(0.56f, 0.54f, 0.50f), shade);
        m_rocks.push_back(r);
    }

    buildSpatialIndex();

    char buffer[160];
    std::snprintf(buffer, sizeof(buffer),
                  "Vegetation scattered: %d grass clusters, %d flowers, %d rocks.",
                  static_cast<int>(m_grass.size()),
                  static_cast<int>(m_flowers.size()),
                  static_cast<int>(m_rocks.size()));
    util::logInfo(buffer);
}

// ---------------------------------------------------------------------------
//  Spatial index
// ---------------------------------------------------------------------------
int Vegetation::cellIndex(float x, float z) const {
    if (m_gridRes <= 0) return -1;
    int cx = static_cast<int>((x - m_gridOrigin) / m_gridCellSize);
    int cz = static_cast<int>((z - m_gridOrigin) / m_gridCellSize);
    if (cx < 0) cx = 0;
    if (cz < 0) cz = 0;
    if (cx > m_gridRes - 1) cx = m_gridRes - 1;
    if (cz > m_gridRes - 1) cz = m_gridRes - 1;
    return cz * m_gridRes + cx;
}

void Vegetation::buildSpatialIndex() {
    m_gridCellSize = 16.0f;
    m_gridOrigin   = -Config::WORLD_SIZE * 0.5f - m_gridCellSize;
    m_gridRes      = static_cast<int>((Config::WORLD_SIZE + m_gridCellSize * 2.0f)
                                      / m_gridCellSize) + 1;

    m_cells.assign(static_cast<size_t>(m_gridRes * m_gridRes), GridCell());

    // Cell bounding spheres, generous enough vertically to contain any tuft.
    for (int cz = 0; cz < m_gridRes; ++cz) {
        for (int cx = 0; cx < m_gridRes; ++cx) {
            GridCell& cell = m_cells[static_cast<size_t>(cz * m_gridRes + cx)];
            cell.center = Vec3(m_gridOrigin + (static_cast<float>(cx) + 0.5f) * m_gridCellSize,
                               0.0f,
                               m_gridOrigin + (static_cast<float>(cz) + 0.5f) * m_gridCellSize);
            cell.radius = m_gridCellSize * 0.75f;
        }
    }

    float minY = 1e9f, maxY = -1e9f;
    for (size_t i = 0; i < m_grass.size(); ++i) {
        int index = cellIndex(m_grass[i].position.x, m_grass[i].position.z);
        if (index < 0) continue;
        m_cells[static_cast<size_t>(index)].grass.push_back(static_cast<int>(i));
        if (m_grass[i].position.y < minY) minY = m_grass[i].position.y;
        if (m_grass[i].position.y > maxY) maxY = m_grass[i].position.y;
    }
    for (size_t i = 0; i < m_flowers.size(); ++i) {
        int index = cellIndex(m_flowers[i].position.x, m_flowers[i].position.z);
        if (index < 0) continue;
        m_cells[static_cast<size_t>(index)].flowers.push_back(static_cast<int>(i));
    }

    // Grow each cell's sphere to cover the terrain height range it spans.
    float midY = (minY + maxY) * 0.5f;
    float halfY = (maxY - minY) * 0.5f + 2.0f;
    for (size_t i = 0; i < m_cells.size(); ++i) {
        m_cells[i].center.y = midY;
        m_cells[i].radius = std::sqrt(m_gridCellSize * m_gridCellSize * 0.5f
                                      + halfY * halfY) + 1.0f;
    }

    util::logInfo("Vegetation spatial index built.");
}

// ---------------------------------------------------------------------------
//  Shared geometry
// ---------------------------------------------------------------------------
void Vegetation::buildRenderData() {
    if (m_listsBuilt) return;

    // Grass comes in three tiers. The tuft only needs its full blade count
    // within about twenty metres; past that the extra geometry is invisible
    // but still costs exactly as much to push.
    struct Tier { GLuint* list; int blades; int segments; };

    m_grassList    = glGenLists(1);
    m_grassMidList = glGenLists(1);
    m_grassFarList = glGenLists(1);

    Tier tiers[3] = {
        { &m_grassList,    10, 3 },   // near  - full tuft
        { &m_grassMidList,  5, 2 },   // mid   - half the blades
        { &m_grassFarList,  3, 1 }    // far   - a suggestion of a tuft
    };

    for (int t = 0; t < 3; ++t) {
        if (!*tiers[t].list) continue;

        // Same seed for every tier, so a tuft keeps its shape as it changes
        // detail level and there is no visible pop when the tier switches.
        util::Rng bladeRng(90210u);

        glNewList(*tiers[t].list, GL_COMPILE);
        for (int i = 0; i < tiers[t].blades; ++i) {
            float yaw    = bladeRng.range(0.0f, 360.0f);
            float lean   = bladeRng.range(-11.0f, 11.0f);
            float height = bladeRng.range(0.15f, 0.30f);
            float bend   = bladeRng.range(0.04f, 0.12f);
            float width  = bladeRng.range(0.026f, 0.044f);
            float offset = bladeRng.range(0.0f, 0.18f);

            glPushMatrix();
            glRotatef(yaw, 0.0f, 1.0f, 0.0f);
            glTranslatef(offset, 0.0f, 0.0f);
            glRotatef(lean, 0.0f, 0.0f, 1.0f);
            prim::drawGrassBlade(width, height, bend, tiers[t].segments);
            glPopMatrix();
        }
        glEndList();
    }

    // Flower: a stem plus a small cross of petals.
    m_flowerList = glGenLists(1);
    if (m_flowerList) {
        glNewList(m_flowerList, GL_COMPILE);
        // A ring of small petals around a centre, at wildflower scale. The
        // previous flattened blob on a tall stem read as a mushroom.
        glPushMatrix();
        glTranslatef(0.0f, 0.205f, 0.0f);
        prim::drawBlob(0.040f, 6, 4, 555u, 0.15f, 0.70f);
        for (int petal = 0; petal < 5; ++petal) {
            float angle = (static_cast<float>(petal) / 5.0f) * 360.0f;
            glPushMatrix();
            glRotatef(angle, 0.0f, 1.0f, 0.0f);
            glTranslatef(0.065f, 0.0f, 0.0f);
            glRotatef(24.0f, 0.0f, 0.0f, -1.0f);
            prim::drawBlob(0.030f, 5, 4, 556u + static_cast<unsigned>(petal),
                           0.12f, 0.38f);
            glPopMatrix();
        }
        glPopMatrix();
        glEndList();
    }

    // Rocks: irregular blobs, each variant a different hash seed.
    const unsigned rockSeeds[ROCK_VARIANT_COUNT] = { 401u, 977u, 1543u, 2711u };
    for (int i = 0; i < ROCK_VARIANT_COUNT; ++i) {
        m_rockLists[i] = glGenLists(1);
        if (!m_rockLists[i]) continue;
        glNewList(m_rockLists[i], GL_COMPILE);
        prim::drawBlob(0.85f, 7, 5, rockSeeds[i], 0.34f,
                       0.62f + static_cast<float>(i) * 0.09f);
        glEndList();
    }

    m_listsBuilt = true;
    util::logInfo("Vegetation geometry compiled into display lists.");
}

void Vegetation::destroyRenderData() {
    if (!m_listsBuilt) return;
    if (m_grassList)    glDeleteLists(m_grassList, 1);
    if (m_grassMidList) glDeleteLists(m_grassMidList, 1);
    if (m_grassFarList) glDeleteLists(m_grassFarList, 1);
    if (m_flowerList)   glDeleteLists(m_flowerList, 1);
    for (int i = 0; i < ROCK_VARIANT_COUNT; ++i)
        if (m_rockLists[i]) glDeleteLists(m_rockLists[i], 1);

    m_grassList = m_grassMidList = m_grassFarList = m_flowerList = 0;
    for (int i = 0; i < ROCK_VARIANT_COUNT; ++i) m_rockLists[i] = 0;
    m_listsBuilt = false;
}

// ---------------------------------------------------------------------------
//  Rendering
// ---------------------------------------------------------------------------
void Vegetation::renderRocks(const RenderContext& context) const {
    if (!m_listsBuilt) return;

    for (size_t i = 0; i < m_rocks.size(); ++i) {
        const RockInstance& rock = m_rocks[i];

        Vec3 centre(rock.position.x, rock.position.y + rock.scale * 0.4f, rock.position.z);
        if (!context.frustum.sphereVisible(centre, rock.scale * 1.4f)) continue;
        if (util::distanceXZ(rock.position, context.cameraPosition) > context.drawDistance) continue;

        ++context.visibleProps;

        glPushMatrix();
        glTranslatef(rock.position.x, rock.position.y + rock.scale * 0.35f, rock.position.z);
        glRotatef(rock.rotation, 0.0f, 1.0f, 0.0f);
        glRotatef(rock.tilt, 1.0f, 0.0f, 0.3f);
        glScalef(rock.scale, rock.scale, rock.scale);
        glColor3f(rock.color.x, rock.color.y, rock.color.z);
        if (m_rockLists[rock.variant]) glCallList(m_rockLists[rock.variant]);
        glPopMatrix();
    }
}

void Vegetation::renderGrassAndFlowers(const RenderContext& context) const {
    if (!m_listsBuilt || m_gridRes <= 0) return;

    // Grass and flowers are thin blades, so both faces must be lit and drawn;
    // culling would make half of every tuft vanish.
    glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT);
    glDisable(GL_CULL_FACE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    const float maxDistance = m_grassDrawDistance;
    const float nearSq = m_grassNearDistance * m_grassNearDistance;
    const float midSq  = m_grassMidDistance  * m_grassMidDistance;
    const float grassSq  = maxDistance * maxDistance;
    const float flowerSq = m_flowerDrawDistance * m_flowerDrawDistance;

    // Only walk the cells that can possibly be in range, instead of every
    // instance on the island.
    int cxMin = static_cast<int>((context.cameraPosition.x - maxDistance - m_gridOrigin) / m_gridCellSize);
    int cxMax = static_cast<int>((context.cameraPosition.x + maxDistance - m_gridOrigin) / m_gridCellSize);
    int czMin = static_cast<int>((context.cameraPosition.z - maxDistance - m_gridOrigin) / m_gridCellSize);
    int czMax = static_cast<int>((context.cameraPosition.z + maxDistance - m_gridOrigin) / m_gridCellSize);

    if (cxMin < 0) cxMin = 0;
    if (czMin < 0) czMin = 0;
    if (cxMax > m_gridRes - 1) cxMax = m_gridRes - 1;
    if (czMax > m_gridRes - 1) czMax = m_gridRes - 1;

    for (int cz = czMin; cz <= czMax; ++cz) {
        for (int cx = cxMin; cx <= cxMax; ++cx) {
            const GridCell& cell = m_cells[static_cast<size_t>(cz * m_gridRes + cx)];
            if (cell.grass.empty() && cell.flowers.empty()) continue;

            // One sphere test rejects everything in this cell. This is the
            // whole point of the grid: work scales with what is on screen.
            if (!context.frustum.sphereVisible(cell.center, cell.radius)) continue;

            float cellDx = cell.center.x - context.cameraPosition.x;
            float cellDz = cell.center.z - context.cameraPosition.z;
            float cellDistance = std::sqrt(cellDx * cellDx + cellDz * cellDz)
                               - m_gridCellSize;
            if (cellDistance > maxDistance) continue;

            // ---------------- grass ----------------
            for (size_t k = 0; k < cell.grass.size(); ++k) {
                const GrassInstance& blade = m_grass[static_cast<size_t>(cell.grass[k])];

                float dx = blade.position.x - context.cameraPosition.x;
                float dz = blade.position.z - context.cameraPosition.z;
                float distanceSq = dx * dx + dz * dz;
                if (distanceSq > grassSq) continue;

                ++context.visibleGrass;

                // Three layered frequencies: a slow lean, a faster ripple, and
                // a twist. One sine alone makes an entire field pulse in unison.
                float sway = std::sin(context.windPhase * 1.6f + blade.phase) * context.wind * 20.0f
                           + std::sin(context.windPhase * 3.7f + blade.phase * 1.9f) * context.wind * 7.0f;
                float swayCross = std::sin(context.windPhase * 1.1f + blade.phase * 0.7f)
                                * context.wind * 9.0f;
                float twist = std::sin(context.windPhase * 0.8f + blade.phase * 1.3f)
                            * context.wind * 12.0f;

                glPushMatrix();
                glTranslatef(blade.position.x, blade.position.y - 0.06f, blade.position.z);
                glRotatef(blade.rotation + twist, 0.0f, 1.0f, 0.0f);
                glRotatef(sway, 0.0f, 0.0f, 1.0f);
                glRotatef(swayCross, 1.0f, 0.0f, 0.0f);
                glScalef(blade.scale, blade.scale, blade.scale);

                glColor3f(blade.color.x, blade.color.y, blade.color.z);
                if      (distanceSq < nearSq && m_grassList)    glCallList(m_grassList);
                else if (distanceSq < midSq  && m_grassMidList) glCallList(m_grassMidList);
                else if (m_grassFarList)                        glCallList(m_grassFarList);

                glPopMatrix();
            }

            // ---------------- flowers ----------------
            if (cellDistance > m_flowerDrawDistance) continue;

            for (size_t k = 0; k < cell.flowers.size(); ++k) {
                const FlowerInstance& flower = m_flowers[static_cast<size_t>(cell.flowers[k])];

                float dx = flower.position.x - context.cameraPosition.x;
                float dz = flower.position.z - context.cameraPosition.z;
                if (dx * dx + dz * dz > flowerSq) continue;

                ++context.visibleProps;

                float sway = std::sin(context.windPhase * 1.9f + flower.phase) * context.wind * 13.0f;

                glPushMatrix();
                glTranslatef(flower.position.x, flower.position.y, flower.position.z);
                glRotatef(flower.rotation, 0.0f, 1.0f, 0.0f);
                glRotatef(sway, 0.0f, 0.0f, 1.0f);
                glScalef(flower.scale, flower.scale, flower.scale);

                glColor3f(0.24f, 0.40f, 0.18f);
                prim::drawCylinder(0.012f, 0.009f, 0.205f, 4, false, false);

                glColor3f(flower.color.x, flower.color.y, flower.color.z);
                if (m_flowerList) glCallList(m_flowerList);
                glPopMatrix();
            }
        }
    }

    glPopAttrib();
}
