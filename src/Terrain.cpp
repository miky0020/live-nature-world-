#include "Terrain.h"
#include "Config.h"

#include <algorithm>
#include <cstdio>

using util::Vec3;

Terrain::Terrain()
    : m_resolution(0)
    , m_size(0.0f)
    , m_cellSize(0.0f)
    , m_seed(0)
    , m_displayList(0)
{
}

Terrain::~Terrain() {
    // No GL calls here: the context may already be gone at static teardown.
}

// ---------------------------------------------------------------------------
//  Height function
//
//  Three layers stacked on top of each other:
//    1. low-frequency fbm      -> broad hills and valleys
//    2. higher-frequency fbm   -> surface texture so slopes are not billiard
//                                 smooth
//    3. radial falloff         -> the land drops below the waterline near the
//                                 edges, which is what makes it an island
//  Then the middle is flattened into a plateau so the campfire, treehouse and
//  characters of later stages have sane ground to stand on.
// ---------------------------------------------------------------------------
float Terrain::computeHeight(float x, float z, unsigned seed) {
    // Value-noise fbm clusters tightly around 0.5, so the amplitudes below are
    // deliberately large: they are what turn a barely-perceptible ripple into
    // readable hills. These numbers were tuned against a height histogram,
    // not guessed.
    float base   = util::fbm2D(x * 0.0095f,         z * 0.0095f,         5, seed);
    float ridge  = util::fbm2D(x * 0.021f + 83.0f,  z * 0.021f - 52.0f,  3, seed + 3u);
    float detail = util::fbm2D(x * 0.045f + 41.0f,  z * 0.045f - 17.0f,  3, seed + 7u);

    float h = (base  - 0.46f) * 54.0f     // broad hills and valleys
            + (ridge - 0.50f) * 18.9f     // secondary ridges
            + (detail - 0.5f) * 3.5f;     // surface texture

    const float half = Config::WORLD_SIZE * 0.5f;
    float distance = std::sqrt(x * x + z * z);
    float normalized = distance / half;

    float island = 1.0f - util::smoothstepf(0.60f, 1.0f, normalized);
    h = h * island - (1.0f - island) * 16.0f;

    float flatten = 1.0f - util::smoothstepf(Config::VILLAGE_RADIUS,
                                             Config::VILLAGE_RADIUS + 20.0f,
                                             distance);
    h = util::lerpf(h, Config::VILLAGE_HEIGHT, flatten * 0.88f);

    return h;
}

// The path network: a ring around the village plus spurs out to the treehouse
// and the gathering area. Defined analytically so nothing has to be stored.
float Terrain::pathInfluence(float x, float z) {
    float distance = std::sqrt(x * x + z * z);

    // Ring
    float ringDistance = std::fabs(distance - Config::PATH_RING_RADIUS);
    float influence = 1.0f - util::smoothstepf(Config::PATH_HALF_WIDTH,
                                               Config::PATH_HALF_WIDTH + 1.1f,
                                               ringDistance);

    // Spur to the treehouse
    float spurA = util::distanceToSegmentXZ(x, z, 0.0f, 0.0f,
                                            Config::TREEHOUSE_X, Config::TREEHOUSE_Z);
    influence = std::max(influence,
                         1.0f - util::smoothstepf(Config::PATH_HALF_WIDTH * 0.8f,
                                                  Config::PATH_HALF_WIDTH * 0.8f + 1.0f,
                                                  spurA));

    // Spur to the gathering area
    float spurB = util::distanceToSegmentXZ(x, z, 0.0f, 0.0f,
                                            Config::DANCE_X, Config::DANCE_Z);
    influence = std::max(influence,
                         1.0f - util::smoothstepf(Config::PATH_HALF_WIDTH * 0.8f,
                                                  Config::PATH_HALF_WIDTH * 0.8f + 1.0f,
                                                  spurB));

    return util::clampf(influence, 0.0f, 1.0f);
}

Vec3 Terrain::computeColor(float height, float slope, float x, float z, unsigned seed) {
    const Vec3 sand   (0.76f, 0.70f, 0.53f);
    const Vec3 grassLo(0.17f, 0.30f, 0.15f);
    const Vec3 grassHi(0.33f, 0.46f, 0.23f);
    const Vec3 rock   (0.40f, 0.38f, 0.36f);

    float tint  = util::fbm2D(x * 0.07f, z * 0.07f, 2, seed + 31u);
    Vec3  color = util::lerpv(grassLo, grassHi, tint);

    // Beach band just above the waterline.
    float beach = 1.0f - util::smoothstepf(Config::WATER_LEVEL + 0.2f,
                                           Config::WATER_LEVEL + 2.4f, height);
    color = util::lerpv(color, sand, beach);

    // Exposed rock on steep faces.
    float rocky = util::smoothstepf(0.45f, 0.75f, slope);
    color = util::lerpv(color, rock, rocky);

    // Worn dirt paths, baked straight into the vertex colours. Drawing them as
    // separate decal geometry would mean fighting the depth buffer for no gain.
    const Vec3 dirt(0.46f, 0.38f, 0.27f);
    float path = pathInfluence(x, z);
    if (height > Config::WATER_LEVEL + 0.5f) {
        color = util::lerpv(color, dirt, path * 0.85f);
    }

    return color;
}

// ---------------------------------------------------------------------------
void Terrain::generate(unsigned seed) {
    m_seed       = seed;
    m_resolution = Config::TERRAIN_RES;
    m_size       = Config::WORLD_SIZE;
    m_cellSize   = m_size / static_cast<float>(m_resolution - 1);

    const int count = m_resolution * m_resolution;
    m_heights.assign(static_cast<size_t>(count), 0.0f);
    m_normals.assign(static_cast<size_t>(count), Vec3(0.0f, 1.0f, 0.0f));
    m_colors.assign(static_cast<size_t>(count), Vec3(0.3f, 0.5f, 0.25f));

    const float half = m_size * 0.5f;

    for (int iz = 0; iz < m_resolution; ++iz) {
        for (int ix = 0; ix < m_resolution; ++ix) {
            float x = -half + static_cast<float>(ix) * m_cellSize;
            float z = -half + static_cast<float>(iz) * m_cellSize;
            m_heights[static_cast<size_t>(iz * m_resolution + ix)] = computeHeight(x, z, seed);
        }
    }

    // Normals from central differences on the baked grid.
    for (int iz = 0; iz < m_resolution; ++iz) {
        for (int ix = 0; ix < m_resolution; ++ix) {
            float hL = rawHeight(ix - 1, iz);
            float hR = rawHeight(ix + 1, iz);
            float hD = rawHeight(ix, iz - 1);
            float hU = rawHeight(ix, iz + 1);

            Vec3 n(hL - hR, 2.0f * m_cellSize, hD - hU);
            n = n.normalized();

            size_t index = static_cast<size_t>(iz * m_resolution + ix);
            m_normals[index] = n;

            float x = -half + static_cast<float>(ix) * m_cellSize;
            float z = -half + static_cast<float>(iz) * m_cellSize;
            float slope = 1.0f - util::clampf(n.y, 0.0f, 1.0f);
            m_colors[index] = computeColor(m_heights[index], slope, x, z, seed);
        }
    }

    char buffer[128];
    std::snprintf(buffer, sizeof(buffer),
                  "Terrain generated: %dx%d vertices over %.0f units.",
                  m_resolution, m_resolution, m_size);
    util::logInfo(buffer);
}

float Terrain::rawHeight(int ix, int iz) const {
    if (m_resolution <= 0) return 0.0f;
    if (ix < 0) ix = 0;
    if (iz < 0) iz = 0;
    if (ix > m_resolution - 1) ix = m_resolution - 1;
    if (iz > m_resolution - 1) iz = m_resolution - 1;
    return m_heights[static_cast<size_t>(iz * m_resolution + ix)];
}

float Terrain::heightAt(float x, float z) const {
    if (m_resolution <= 0) return 0.0f;

    const float half = m_size * 0.5f;
    float gx = (x + half) / m_cellSize;
    float gz = (z + half) / m_cellSize;

    int ix = static_cast<int>(std::floor(gx));
    int iz = static_cast<int>(std::floor(gz));
    float tx = gx - static_cast<float>(ix);
    float tz = gz - static_cast<float>(iz);

    float h00 = rawHeight(ix,     iz);
    float h10 = rawHeight(ix + 1, iz);
    float h01 = rawHeight(ix,     iz + 1);
    float h11 = rawHeight(ix + 1, iz + 1);

    return util::lerpf(util::lerpf(h00, h10, tx),
                       util::lerpf(h01, h11, tx), tz);
}

Vec3 Terrain::normalAt(float x, float z) const {
    float step = m_cellSize > 0.0f ? m_cellSize : 1.0f;
    float hL = heightAt(x - step, z);
    float hR = heightAt(x + step, z);
    float hD = heightAt(x, z - step);
    float hU = heightAt(x, z + step);
    return Vec3(hL - hR, 2.0f * step, hD - hU).normalized();
}

bool Terrain::isInsideWorld(float x, float z) const {
    const float half = m_size * 0.5f;
    return x > -half && x < half && z > -half && z < half;
}

// ---------------------------------------------------------------------------
void Terrain::buildRenderData() {
    if (m_resolution <= 0) return;
    destroyRenderData();

    m_displayList = glGenLists(1);
    if (m_displayList == 0) {
        util::logWarning("Could not allocate a display list for the terrain; "
                         "falling back to immediate mode each frame.");
        return;
    }

    const float half = m_size * 0.5f;

    glNewList(m_displayList, GL_COMPILE);
    for (int iz = 0; iz < m_resolution - 1; ++iz) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int ix = 0; ix < m_resolution; ++ix) {
            for (int row = 0; row < 2; ++row) {
                int zz = iz + row;
                size_t index = static_cast<size_t>(zz * m_resolution + ix);
                const Vec3& n = m_normals[index];
                const Vec3& c = m_colors[index];
                float x = -half + static_cast<float>(ix) * m_cellSize;
                float z = -half + static_cast<float>(zz) * m_cellSize;
                glColor3f(c.x, c.y, c.z);
                glNormal3f(n.x, n.y, n.z);
                glVertex3f(x, m_heights[index], z);
            }
        }
        glEnd();
    }
    glEndList();

    util::logInfo("Terrain mesh compiled into a display list.");
}

void Terrain::destroyRenderData() {
    if (m_displayList != 0) {
        glDeleteLists(m_displayList, 1);
        m_displayList = 0;
    }
}

void Terrain::render() const {
    if (m_displayList != 0) {
        glCallList(m_displayList);
        return;
    }

    // Fallback path if the display list could not be created.
    if (m_resolution <= 0) return;
    const float half = m_size * 0.5f;
    for (int iz = 0; iz < m_resolution - 1; ++iz) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int ix = 0; ix < m_resolution; ++ix) {
            for (int row = 0; row < 2; ++row) {
                int zz = iz + row;
                size_t index = static_cast<size_t>(zz * m_resolution + ix);
                const Vec3& n = m_normals[index];
                const Vec3& c = m_colors[index];
                glColor3f(c.x, c.y, c.z);
                glNormal3f(n.x, n.y, n.z);
                glVertex3f(-half + static_cast<float>(ix) * m_cellSize,
                           m_heights[index],
                           -half + static_cast<float>(zz) * m_cellSize);
            }
        }
        glEnd();
    }
}
