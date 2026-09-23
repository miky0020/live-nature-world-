#include "Water.h"
#include "Config.h"
#include "Terrain.h"

using util::Vec3;

Water::Water()
    : m_level(Config::WATER_LEVEL)
    , m_extent(Config::WORLD_SIZE * 1.6f)
    , m_resolution(36)
    , m_cellSize(1.0f)
    , m_time(0.0f)
    , m_waveAmplitude(0.16f)
{
}

void Water::initialize(float extent, int resolution) {
    m_extent     = extent;
    m_resolution = resolution < 4 ? 4 : resolution;
    m_cellSize   = m_extent / static_cast<float>(m_resolution);
    m_level      = Config::WATER_LEVEL;

    util::logInfo("Water surface initialized.");
}

void Water::update(float deltaTime, float windStrength) {
    m_time += deltaTime * (0.9f + windStrength * 1.8f);

    m_waveAmplitude = util::lerpf(
        0.16f,
        0.52f,
        util::clampf(windStrength, 0.0f, 1.0f)
    );
}

float Water::surfaceHeight(float x, float z) const {
    float a = std::sin(x * 0.055f + m_time * 1.15f);
    float b = std::sin(z * 0.042f - m_time * 0.83f);
    float c = std::sin((x + z) * 0.028f + m_time * 0.55f);

    return m_level
         + (a + b + c * 0.6f)
         * m_waveAmplitude
         * 0.45f;
}

Vec3 Water::surfaceNormal(float x, float z) const {
    const float scale = m_waveAmplitude * 0.45f;

    float dx =
        (std::cos(x * 0.055f + m_time * 1.15f) * 0.055f
        + std::cos((x + z) * 0.028f + m_time * 0.55f)
          * 0.028f * 0.6f) * scale;

    float dz =
        (std::cos(z * 0.042f - m_time * 0.83f) * 0.042f
        + std::cos((x + z) * 0.028f + m_time * 0.55f)
          * 0.028f * 0.6f) * scale;

    const float relief = 18.0f;

    return Vec3(
        -dx * relief,
        1.0f,
        -dz * relief
    ).normalized();
}

void Water::render(const Vec3& cameraPosition,
                   const Vec3& waterColor,
                   const Vec3& sunDirection,
                   const Vec3& sunColor,
                   float daylight,
                   float alpha,
                   const Terrain& terrain) const
{
    const float half = m_extent * 0.5f;

    const float originX =
        std::floor(cameraPosition.x / m_cellSize) * m_cellSize;

    const float originZ =
        std::floor(cameraPosition.z / m_cellSize) * m_cellSize;

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);

    glDisable(GL_LIGHTING);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDepthMask(GL_FALSE);

    Vec3 sun = sunDirection.normalized();

    float dayAmount =
        util::clampf(daylight, 0.0f, 1.0f);

    Vec3 skyTint = util::lerpv(
        Vec3(0.10f, 0.13f, 0.20f),
        Vec3(0.55f, 0.70f, 0.88f),
        dayAmount
    );

    for (int iz = 0; iz < m_resolution; ++iz) {
        float z0 =
            originZ - half
            + static_cast<float>(iz) * m_cellSize;

        float z1 =
            originZ - half
            + static_cast<float>(iz + 1) * m_cellSize;

        glBegin(GL_TRIANGLE_STRIP);

        for (int ix = 0; ix <= m_resolution; ++ix) {
            float x =
                originX - half
                + static_cast<float>(ix) * m_cellSize;

            for (int row = 0; row < 2; ++row) {
                float z = (row == 0) ? z0 : z1;

                float y = surfaceHeight(x, z);
                Vec3 n = surfaceNormal(x, z);

                Vec3 toEye =
                    (cameraPosition - Vec3(x, y, z)).normalized();

                float facing =
                    util::clampf(
                        util::dot(n, toEye),
                        0.0f,
                        1.0f
                    );

                float fresnel =
                    0.10f
                    + 0.90f
                    * std::pow(1.0f - facing, 4.0f);

                Vec3 halfway =
                    (toEye + sun).normalized();

                float specular =
                    util::clampf(
                        util::dot(n, halfway),
                        0.0f,
                        1.0f
                    );

                specular =
                    std::pow(specular, 40.0f)
                    * dayAmount;

                float diffuse =
                    0.62f
                    + 0.38f
                    * util::clampf(
                        util::dot(n, sun),
                        0.0f,
                        1.0f
                    );

                Vec3 color = waterColor * diffuse;

                color =
                    util::lerpv(
                        color,
                        skyTint,
                        fresnel * 0.55f
                    );

                color +=
                    sunColor * (specular * 0.95f);

                // ---------------------------------------------------------
                // Shoreline foam
                // ---------------------------------------------------------
                float landHeight =
                    terrain.heightAt(x, z);

                float depth =
                    y - landHeight;

                float foam =
                    1.0f
                    - util::clampf(
                        depth / 0.85f,
                        0.0f,
                        1.0f
                    );

                foam *= foam;

                color =
                    util::lerpv(
                        color,
                        Vec3(0.92f, 0.95f, 0.97f),
                        foam * 0.75f
                    );

                float a =
                    util::clampf(
                        alpha
                        + fresnel * 0.15f
                        + foam * 0.30f,
                        0.0f,
                        0.98f
                    );

                glColor4f(
                    util::clampf(color.x, 0.0f, 1.0f),
                    util::clampf(color.y, 0.0f, 1.0f),
                    util::clampf(color.z, 0.0f, 1.0f),
                    a
                );

                glVertex3f(x, y, z);
            }
        }

        glEnd();
    }

    glDepthMask(GL_TRUE);

    glPopAttrib();
}