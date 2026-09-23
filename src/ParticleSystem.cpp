#include "ParticleSystem.h"
#include "Config.h"
#include "GLIncludes.h"
#include "Terrain.h"

using util::Vec3;

// ============================================================ ParticlePool ==
ParticlePool::ParticlePool() : m_cursor(0), m_aliveCount(0) {}

void ParticlePool::initialize(int capacity) {
    if (capacity < 1) capacity = 1;
    m_particles.assign(static_cast<size_t>(capacity), Particle());
    m_cursor = 0;
    m_aliveCount = 0;
}

void ParticlePool::reset() {
    for (size_t i = 0; i < m_particles.size(); ++i) m_particles[i].alive = false;
    m_cursor = 0;
    m_aliveCount = 0;
}

Particle* ParticlePool::acquire() {
    const int total = static_cast<int>(m_particles.size());
    if (total == 0 || m_aliveCount >= total) return 0;

    for (int scanned = 0; scanned < total; ++scanned) {
        int index = m_cursor;
        m_cursor = (m_cursor + 1) % total;

        Particle& particle = m_particles[static_cast<size_t>(index)];
        if (!particle.alive) {
            particle.alive = true;
            ++m_aliveCount;
            return &particle;
        }
    }
    return 0;
}

void ParticlePool::kill(Particle& particle) {
    if (particle.alive) {
        particle.alive = false;
        --m_aliveCount;
        if (m_aliveCount < 0) m_aliveCount = 0;
    }
}

// ============================================================= RainSystem ===
RainSystem::RainSystem()
    : m_rng(Config::WORLD_SEED ^ 0x7A17A1u)
    , m_spawnAccumulator(0.0f)
{
}

void RainSystem::initialize(int capacity) {
    m_pool.initialize(capacity);
    m_spawnAccumulator = 0.0f;
    util::logInfo("Rain particle pool allocated.");
}

void RainSystem::update(float deltaTime, const RenderContext& context, const Terrain& terrain) {
    const float spawnRadius = 46.0f;
    const float spawnHeight = 30.0f;

    // How many particles a full-intensity shower should keep in the air.
    const float targetAlive = static_cast<float>(m_pool.capacity()) * context.rainIntensity;

    if (context.rainIntensity <= 0.001f && m_pool.aliveCount() == 0) {
        m_spawnAccumulator = 0.0f;
        return;
    }

    // --- spawn -------------------------------------------------------------
    if (context.rainIntensity > 0.001f) {
        float deficit = targetAlive - static_cast<float>(m_pool.aliveCount());
        if (deficit > 0.0f) {
            // Refill over roughly a third of a second rather than in one burst.
            m_spawnAccumulator += deficit * 3.0f * deltaTime;
            int toSpawn = static_cast<int>(m_spawnAccumulator);
            if (toSpawn > 0) {
                m_spawnAccumulator -= static_cast<float>(toSpawn);
                for (int i = 0; i < toSpawn; ++i) {
                    Particle* p = m_pool.acquire();
                    if (!p) break;

                    float angle  = m_rng.range(0.0f, util::TWO_PI);
                    float radius = std::sqrt(m_rng.nextFloat()) * spawnRadius;

                    p->position = Vec3(context.cameraPosition.x + std::cos(angle) * radius,
                                       context.cameraPosition.y + m_rng.range(6.0f, spawnHeight),
                                       context.cameraPosition.z + std::sin(angle) * radius);

                    float fallSpeed = -(20.0f + context.rainIntensity * 14.0f
                                             + m_rng.range(-2.5f, 2.5f));
                    p->velocity  = Vec3(0.0f, fallSpeed, 0.0f);
                    p->maxLife   = 6.0f;
                    p->life      = p->maxLife;
                    p->size      = m_rng.range(0.55f, 1.25f);
                    p->variation = m_rng.nextFloat();
                }
            }
        }
    }

    // --- integrate ---------------------------------------------------------
    // Wind pushes rain sideways; section 6 asks for exactly this coupling.
    float windPush = context.wind * 11.0f;
    Vec3 windVector(windPush * 0.85f, 0.0f, windPush * 0.35f);

    const int total = m_pool.capacity();
    for (int i = 0; i < total; ++i) {
        Particle& p = m_pool.at(i);
        if (!p.alive) continue;

        p.velocity.x = util::damp(p.velocity.x, windVector.x, 2.0f, deltaTime);
        p.velocity.z = util::damp(p.velocity.z, windVector.z, 2.0f, deltaTime);

        p.position += p.velocity * deltaTime;
        p.life -= deltaTime;

        // Recycle on contact with the ground, the sea, or on timeout - and
        // also if the camera has flown far enough that this drop is no longer
        // anywhere near the player.
        float ground = terrain.heightAt(p.position.x, p.position.z);
        float floorY = ground > Config::WATER_LEVEL ? ground : Config::WATER_LEVEL;

        bool tooFar = util::distanceXZ(p.position, context.cameraPosition) > spawnRadius * 1.6f;

        if (p.life <= 0.0f || p.position.y <= floorY || tooFar) {
            m_pool.kill(p);
        }
    }

    // If intensity dropped, let the surplus fall out naturally rather than
    // vanishing mid-air.
    context.liveParticles += m_pool.aliveCount();
}

void RainSystem::render(const RenderContext& context) const {
    if (m_pool.aliveCount() == 0) return;

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_LINE_BIT
                | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);       // drops must not occlude each other
    glLineWidth(1.4f);

    // Streak colour leans toward the sky so rain reads correctly at night too.
    float brightness = 0.45f + context.daylight * 0.45f;
    float alpha = 0.22f + context.rainIntensity * 0.38f;

    glBegin(GL_LINES);
    const int total = m_pool.capacity();
    for (int i = 0; i < total; ++i) {
        const Particle& p = m_pool.at(i);
        if (!p.alive) continue;

        // Streak along the velocity vector: that is what makes it read as
        // falling rain rather than floating dots.
        Vec3 tail = p.position - p.velocity * 0.035f;

        glColor4f(brightness * 0.78f, brightness * 0.86f, brightness,
                  alpha * (0.6f + p.variation * 0.4f));
        glVertex3f(p.position.x, p.position.y, p.position.z);
        glColor4f(brightness * 0.78f, brightness * 0.86f, brightness, 0.02f);
        glVertex3f(tail.x, tail.y, tail.z);
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glPopAttrib();
}

// ============================================================ SparkSystem ===
SparkSystem::SparkSystem()
    : m_rng(Config::WORLD_SEED ^ 0x5A9C0Fu)
    , m_origin(0.0f, 0.0f, 0.0f)
    , m_spawnAccumulator(0.0f)
{
}

void SparkSystem::initialize(int capacity) {
    m_pool.initialize(capacity);
    m_spawnAccumulator = 0.0f;
}

void SparkSystem::update(float deltaTime, const RenderContext& context, float emissionRate) {
    m_spawnAccumulator += emissionRate * deltaTime;
    int toSpawn = static_cast<int>(m_spawnAccumulator);
    if (toSpawn > 0) {
        m_spawnAccumulator -= static_cast<float>(toSpawn);
        for (int i = 0; i < toSpawn; ++i) {
            Particle* p = m_pool.acquire();
            if (!p) break;

            float angle  = m_rng.range(0.0f, util::TWO_PI);
            float radius = m_rng.range(0.0f, 0.32f);

            p->position = m_origin + Vec3(std::cos(angle) * radius,
                                          m_rng.range(0.15f, 0.55f),
                                          std::sin(angle) * radius);
            p->velocity = Vec3(m_rng.range(-0.35f, 0.35f),
                               m_rng.range(1.5f, 3.4f),
                               m_rng.range(-0.35f, 0.35f));
            p->maxLife   = m_rng.range(0.8f, 1.9f);
            p->life      = p->maxLife;
            p->size      = m_rng.range(1.5f, 3.6f);
            p->variation = m_rng.nextFloat();
        }
    }

    Vec3 wind(context.wind * 1.9f, 0.0f, context.wind * 0.8f);

    const int total = m_pool.capacity();
    for (int i = 0; i < total; ++i) {
        Particle& p = m_pool.at(i);
        if (!p.alive) continue;

        // Sparks slow as they rise and cool, and drift with the wind.
        p.velocity.y -= 0.85f * deltaTime;
        p.velocity.x = util::damp(p.velocity.x, wind.x, 1.4f, deltaTime);
        p.velocity.z = util::damp(p.velocity.z, wind.z, 1.4f, deltaTime);

        p.position += p.velocity * deltaTime;
        p.life -= deltaTime;

        if (p.life <= 0.0f) m_pool.kill(p);
    }

    context.liveParticles += m_pool.aliveCount();
}

void SparkSystem::render(const RenderContext& context) const {
    if (m_pool.aliveCount() == 0) return;
    (void)context;

    // GL_COLOR_BUFFER_BIT included so the additive blend function used for
    // embers cannot outlive this function - see World::render for the full
    // story of what happens when a blend function leaks between draw calls.
    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_POINT_BIT
                | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);      // additive: embers glow
    glDepthMask(GL_FALSE);
    glEnable(GL_POINT_SMOOTH);

    const int total = m_pool.capacity();
    for (int i = 0; i < total; ++i) {
        const Particle& p = m_pool.at(i);
        if (!p.alive) continue;

        float t = util::clampf(p.life / p.maxLife, 0.0f, 1.0f);

        // Fade from hot yellow through orange to a dim red as the ember cools.
        float r = 1.0f;
        float g = 0.35f + t * 0.55f;
        float b = 0.08f + t * t * 0.25f;

        glPointSize(p.size * (0.4f + t * 0.6f));
        glBegin(GL_POINTS);
            glColor4f(r, g, b, t * 0.9f);
            glVertex3f(p.position.x, p.position.y, p.position.z);
        glEnd();
    }

    glDepthMask(GL_TRUE);
    glPopAttrib();
}

// ============================================================ SmokeSystem ===
SmokeSystem::SmokeSystem()
    : m_rng(Config::WORLD_SEED ^ 0x3C4B21u)
    , m_origin(0.0f, 0.0f, 0.0f)
    , m_spawnAccumulator(0.0f)
{
}

void SmokeSystem::initialize(int capacity) {
    m_pool.initialize(capacity);
    m_spawnAccumulator = 0.0f;
}

void SmokeSystem::update(float deltaTime, const RenderContext& context, float emissionRate) {
    m_spawnAccumulator += emissionRate * deltaTime;
    int toSpawn = static_cast<int>(m_spawnAccumulator);
    if (toSpawn > 0) {
        m_spawnAccumulator -= static_cast<float>(toSpawn);
        for (int i = 0; i < toSpawn; ++i) {
            Particle* p = m_pool.acquire();
            if (!p) break;

            float angle  = m_rng.range(0.0f, util::TWO_PI);
            float radius = m_rng.range(0.0f, 0.20f);

            p->position = m_origin + Vec3(std::cos(angle) * radius,
                                          m_rng.range(0.0f, 0.25f),
                                          std::sin(angle) * radius);
            p->velocity = Vec3(m_rng.range(-0.15f, 0.15f),
                               m_rng.range(0.65f, 1.15f),
                               m_rng.range(-0.15f, 0.15f));
            p->maxLife   = m_rng.range(2.2f, 3.6f);
            p->life      = p->maxLife;
            p->size      = m_rng.range(0.35f, 0.65f);
            p->variation = m_rng.nextFloat();
        }
    }

    Vec3 wind(context.wind * 2.4f, 0.0f, context.wind * 1.1f);

    const int total = m_pool.capacity();
    for (int i = 0; i < total; ++i) {
        Particle& p = m_pool.at(i);
        if (!p.alive) continue;

        // Smoke slows and spreads as it rises and cools, drifting with wind.
        p.velocity.y = util::damp(p.velocity.y, 0.25f, 0.6f, deltaTime);
        p.velocity.x = util::damp(p.velocity.x, wind.x, 0.9f, deltaTime);
        p.velocity.z = util::damp(p.velocity.z, wind.z, 0.9f, deltaTime);

        p.position += p.velocity * deltaTime;
        p.life -= deltaTime;

        if (p.life <= 0.0f) m_pool.kill(p);
    }

    context.liveParticles += m_pool.aliveCount();
}

void SmokeSystem::render(const RenderContext& context) const {
    if (m_pool.aliveCount() == 0) return;
    (void)context;

    // Normal (not additive) blending - smoke occludes rather than glows.
    // GL_COLOR_BUFFER_BIT guards the blend function the same way every other
    // transparent effect in this project does.
    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_POINT_BIT
                | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_POINT_SMOOTH);

    const int total = m_pool.capacity();
    for (int i = 0; i < total; ++i) {
        const Particle& p = m_pool.at(i);
        if (!p.alive) continue;

        float t = util::clampf(p.life / p.maxLife, 0.0f, 1.0f);

        // Fades in from nothing, peaks mid-life, thins out as it disperses;
        // grows steadily the whole time so it reads as billowing, not popping.
        float fade  = std::sin((1.0f - t) * util::PI);
        float scale = p.size * (1.0f + (1.0f - t) * 2.2f);
        float shade = 0.30f + p.variation * 0.12f;

        glPointSize(scale * 22.0f);
        glBegin(GL_POINTS);
            glColor4f(shade, shade, shade * 1.02f, fade * 0.22f);
            glVertex3f(p.position.x, p.position.y, p.position.z);
        glEnd();
    }

    glDepthMask(GL_TRUE);
    glPopAttrib();
}



// ======================================================= SakuraPetalSystem ===
SakuraPetalSystem::SakuraPetalSystem()
    : m_rng(Config::WORLD_SEED ^ 0xB1055u)
    , m_origin(0.0f, 0.0f, 0.0f)
    , m_spawnAccumulator(0.0f)
{
}

void SakuraPetalSystem::initialize(const Vec3& origin, unsigned seed, int capacity) {
    m_pool.initialize(capacity);
    m_rng.reseed(seed ^ 0xB1055u);
    m_origin = origin;
    m_spawnAccumulator = 0.0f;

    // Start with a few petals already in the air so the tree never looks
    // frozen during the first seconds of the demo.
    for (int i = 0; i < capacity / 3; ++i) {
        Particle* p = m_pool.acquire();
        if (!p) break;
        // Seed the pool from actual blossom-canopy regions, not from a
        // generic cylinder above the tree.
        const Vec3 sources[] = {
            Vec3( 1.3f, 5.55f,  0.1f), Vec3( 2.5f, 5.95f, -0.2f),
            Vec3( 3.7f, 5.75f,  0.2f), Vec3(-1.5f, 5.85f,  0.0f),
            Vec3(-2.6f, 6.30f, -0.3f), Vec3( 0.0f, 6.85f,  0.2f),
            Vec3( 1.1f, 7.25f, -0.9f), Vec3(-0.9f, 7.15f,  0.8f)
        };
        Vec3 source = sources[m_rng.rangeInt(0, 7)];
        p->position = m_origin + source + Vec3(
            m_rng.range(-0.38f, 0.38f), m_rng.range(-0.28f, 0.28f),
            m_rng.range(-0.38f, 0.38f));
        p->velocity = Vec3(m_rng.range(-0.16f, 0.16f),
                           -m_rng.range(0.20f, 0.52f),
                           m_rng.range(-0.16f, 0.16f));
        p->maxLife = m_rng.range(8.0f, 15.0f);
        p->life = m_rng.range(2.0f, p->maxLife);
        p->size = m_rng.range(2.0f, 4.0f);
        p->variation = m_rng.nextFloat();
    }
}

void SakuraPetalSystem::update(float deltaTime, const RenderContext& context,
                               const Terrain& terrain) {
    // Keep the pool intentionally small: this is a visual accent, not a
    // second weather system.
    m_spawnAccumulator += (4.5f + context.wind * 1.8f) * deltaTime;
    int toSpawn = static_cast<int>(m_spawnAccumulator);
    if (toSpawn > 0) {
        // Small timing jitter keeps the stream organic without ever creating
        // a burst large enough to hurt performance.
        m_spawnAccumulator -= static_cast<float>(toSpawn);
        for (int i = 0; i < toSpawn; ++i) {
            Particle* p = m_pool.acquire();
            if (!p) break;

            const Vec3 sources[] = {
                Vec3( 1.3f, 5.55f,  0.1f), Vec3( 2.5f, 5.95f, -0.2f),
                Vec3( 3.7f, 5.75f,  0.2f), Vec3(-1.5f, 5.85f,  0.0f),
                Vec3(-2.6f, 6.30f, -0.3f), Vec3( 0.0f, 6.85f,  0.2f),
                Vec3( 1.1f, 7.25f, -0.9f), Vec3(-0.9f, 7.15f,  0.8f)
            };
            Vec3 source = sources[m_rng.rangeInt(0, 7)];
            p->position = m_origin + source + Vec3(
                m_rng.range(-0.42f, 0.42f), m_rng.range(-0.30f, 0.30f),
                m_rng.range(-0.42f, 0.42f));

            float ground = terrain.heightAt(p->position.x, p->position.z);
            if (p->position.y < ground + 0.6f) p->position.y = ground + 0.6f;

            p->velocity = Vec3(m_rng.range(-0.16f, 0.16f) + context.wind * 0.24f,
                               -m_rng.range(0.20f, 0.42f),
                               m_rng.range(-0.16f, 0.16f) + context.wind * 0.10f);
            p->maxLife = m_rng.range(7.0f, 13.0f);
            p->life = p->maxLife;
            p->size = m_rng.range(2.5f, 5.0f);
            p->variation = m_rng.nextFloat();

            // Jitter the next emission threshold slightly so timing is not
            // mechanically metronomic.
            m_spawnAccumulator -= m_rng.range(-0.12f, 0.12f);
        }
    }

    for (int i = 0; i < m_pool.capacity(); ++i) {
        Particle& p = m_pool.at(i);
        if (!p.alive) continue;

        float phase = (1.0f - p.life / p.maxLife) * util::TWO_PI;
        p.velocity.x += std::sin(phase * 1.7f + p.variation * 5.0f) * 0.018f;
        p.velocity.z += std::cos(phase * 1.3f + p.variation * 4.0f) * 0.018f;
        p.velocity.x = util::damp(p.velocity.x, context.wind * 0.30f, 1.2f, deltaTime);
        p.velocity.z = util::damp(p.velocity.z, context.wind * 0.10f, 1.2f, deltaTime);
        p.position += p.velocity * deltaTime;
        p.life -= deltaTime;

        float ground = terrain.heightAt(p.position.x, p.position.z);
        if (p.life <= 0.0f || p.position.y <= ground + 0.04f ||
            util::distanceXZ(p.position, m_origin) > Config::SAKURA_PETAL_RADIUS * 1.55f) {
            m_pool.kill(p);
        }
    }

    context.liveParticles += m_pool.aliveCount();
}

void SakuraPetalSystem::render(const RenderContext& context) const {
    if (m_pool.aliveCount() == 0) return;

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_POINT_BIT |
                 GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_FOG_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_POINT_SMOOTH);

    for (int i = 0; i < m_pool.capacity(); ++i) {
        const Particle& p = m_pool.at(i);
        if (!p.alive) continue;
        if (!context.frustum.sphereVisible(p.position, 0.18f)) continue;
        if (util::distanceXZ(p.position, context.cameraPosition) > 24.0f) continue;

        float lifeFade = util::clampf(p.life / 1.2f, 0.0f, 1.0f);
        float alpha = 0.30f + 0.38f * lifeFade;
        float pink = 0.48f + p.variation * 0.28f;
        float s = 0.018f * p.size;
        glColor4f(1.0f, pink, 0.66f + p.variation * 0.24f, alpha);

        // Two crossed quads make a tiny readable petal/flower shape from
        // most camera angles while remaining much cheaper than a mesh.
        glBegin(GL_QUADS);
            glVertex3f(p.position.x - s, p.position.y, p.position.z);
            glVertex3f(p.position.x, p.position.y + s * 0.55f, p.position.z + s * 0.55f);
            glVertex3f(p.position.x + s, p.position.y, p.position.z);
            glVertex3f(p.position.x, p.position.y - s * 0.55f, p.position.z - s * 0.55f);

            glVertex3f(p.position.x, p.position.y, p.position.z - s);
            glVertex3f(p.position.x + s * 0.55f, p.position.y + s * 0.55f, p.position.z);
            glVertex3f(p.position.x, p.position.y, p.position.z + s);
            glVertex3f(p.position.x - s * 0.55f, p.position.y - s * 0.55f, p.position.z);
        glEnd();
    }

    glDepthMask(GL_TRUE);
    glPopAttrib();
}

// ============================================================ FireflySystem ===
FireflySystem::FireflySystem()
    : m_rng(Config::WORLD_SEED ^ 0xF1E57A9u)
    , m_seed(Config::WORLD_SEED)
    , m_time(0.0f)
{
}

void FireflySystem::initialize(const Terrain& terrain, unsigned seed, int count) {
    if (count < 1) count = 1;
    m_seed = seed;
    m_rng.reseed(seed ^ 0xF1E57A9u);
    m_time = 0.0f;
    m_fireflies.assign(static_cast<size_t>(count), Firefly());

    // Concentrate the glow where the reference scene has its strongest
    // nighttime detail: around the treehouse tree, campfire meadow and
    // a few natural clearings around the island.
    const Vec3 centres[] = {
        Vec3(Config::TREEHOUSE_X, 0.0f, Config::TREEHOUSE_Z),
        Vec3(Config::CAMPFIRE_X, 0.0f, Config::CAMPFIRE_Z),
        Vec3(Config::SAKURA_X, 0.0f, Config::SAKURA_Z),
        Vec3(20.0f, 0.0f, 22.0f),
        Vec3(-18.0f, 0.0f, -28.0f)
    };
    const int centreCount = static_cast<int>(sizeof(centres) / sizeof(centres[0]));

    for (size_t i = 0; i < m_fireflies.size(); ++i) {
        Firefly& f = m_fireflies[i];
        int c = static_cast<int>(i % static_cast<size_t>(centreCount));
        Vec3 base = centres[c];

        float angle = m_rng.range(0.0f, util::TWO_PI);
        float distance = std::sqrt(m_rng.nextFloat()) * 7.5f;
        float x = base.x + std::cos(angle) * distance;
        float z = base.z + std::sin(angle) * distance;

        // Keep the particles above the actual terrain, even on sloped
        // clearings. The vertical offset gives the soft floating look.
        float ground = terrain.heightAt(x, z);

        f.anchor = Vec3(x, ground + m_rng.range(0.25f, 1.45f), z);
        f.position = f.anchor;
        f.phase = m_rng.range(0.0f, util::TWO_PI);
        f.speed = m_rng.range(0.45f, 1.15f);
        f.radius = m_rng.range(0.25f, 1.25f);
        f.height = m_rng.range(0.18f, 0.65f);
        f.alive = true;
    }

    util::logInfo("Fireflies initialized around trees and flower clearings.");
}

void FireflySystem::update(float deltaTime, const RenderContext& context) {
    m_time += deltaTime;

    for (size_t i = 0; i < m_fireflies.size(); ++i) {
        Firefly& f = m_fireflies[i];

        float t = m_time * f.speed + f.phase;
        float orbit = t * 0.72f;
        float bob = std::sin(t * 1.7f) * f.height;
        float driftX = std::sin(t * 0.91f + f.phase * 1.7f) * f.radius;
        float driftZ = std::cos(t * 0.83f + f.phase * 1.3f) * f.radius;

        f.position = f.anchor
                   + Vec3(std::cos(orbit) * f.radius * 0.65f + driftX * 0.35f,
                          bob,
                          std::sin(orbit) * f.radius * 0.65f + driftZ * 0.35f);
    }

    // Keep the shared particle counter honest for the debug HUD.
    context.liveParticles += static_cast<int>(m_fireflies.size());
}

void FireflySystem::render(const RenderContext& context) const {
    if (m_fireflies.empty()) return;

    // Fireflies are an emissive effect: additive blending creates a small
    // glowing core without needing another fixed-function light.
    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_POINT_BIT
                | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_FOG_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glEnable(GL_POINT_SMOOTH);

    float night = util::clampf(context.nightFactor, 0.0f, 1.0f);
    float dusk = 1.0f - std::fabs(night - 0.5f) * 2.0f;
    dusk = util::clampf(dusk, 0.0f, 1.0f);
    float visibility = util::clampf(night * 1.35f + dusk * 0.08f, 0.0f, 1.0f);

    if (visibility > 0.001f) {
        for (size_t i = 0; i < m_fireflies.size(); ++i) {
            const Firefly& f = m_fireflies[i];
            if (!context.frustum.sphereVisible(f.position, 0.35f)) continue;
            if (util::distanceXZ(f.position, context.cameraPosition) > 55.0f) continue;

            float pulse = 0.45f + 0.55f
                        * (0.5f + 0.5f * std::sin(m_time * (2.0f + f.speed)
                                                   + f.phase));
            float alpha = visibility * pulse * 0.95f;

            // Soft halo first, then a tiny hot core.
            glPointSize(9.0f);
            glBegin(GL_POINTS);
                glColor4f(0.55f, 1.0f, 0.16f, alpha * 0.13f);
                glVertex3f(f.position.x, f.position.y, f.position.z);
            glEnd();

            glPointSize(3.0f);
            glBegin(GL_POINTS);
                glColor4f(0.75f, 1.0f, 0.28f, alpha);
                glVertex3f(f.position.x, f.position.y, f.position.z);
            glEnd();
        }
    }

    glDepthMask(GL_TRUE);
    glPopAttrib();
}
