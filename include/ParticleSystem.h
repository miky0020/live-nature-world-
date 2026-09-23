// ============================================================================
//  ParticleSystem.h - fixed-size particle pools.
//
//  Section 12 is explicit: no allocation or deletion at runtime. Both systems
//  below allocate their whole pool once at startup and then only ever recycle
//  slots, so a steady-state frame does zero heap work.
// ============================================================================
#ifndef LIVINGISLAND_PARTICLESYSTEM_H
#define LIVINGISLAND_PARTICLESYSTEM_H

#include "RenderContext.h"
#include "Utilities.h"

#include <vector>

class Terrain;

struct Particle {
    util::Vec3 position;
    util::Vec3 velocity;
    float life;        // seconds remaining
    float maxLife;
    float size;
    float variation;   // per-particle 0..1, used for colour and size jitter
    bool  alive;

    Particle() : life(0.0f), maxLife(1.0f), size(1.0f), variation(0.0f), alive(false) {}
};

// ---------------------------------------------------------------------------
//  Shared storage. Keeps a rolling cursor so finding a free slot is O(1) in
//  practice rather than a scan from the start every spawn.
// ---------------------------------------------------------------------------
class ParticlePool {
public:
    ParticlePool();

    void initialize(int capacity);
    void reset();

    Particle* acquire();                      // null when the pool is full
    int  capacity() const { return static_cast<int>(m_particles.size()); }
    int  aliveCount() const { return m_aliveCount; }

    Particle&       at(int index)       { return m_particles[static_cast<size_t>(index)]; }
    const Particle& at(int index) const { return m_particles[static_cast<size_t>(index)]; }
    void kill(Particle& particle);

private:
    std::vector<Particle> m_particles;
    int m_cursor;
    int m_aliveCount;
};

// ---------------------------------------------------------------------------
//  Rain. Particles spawn in a box that travels with the camera, so the player
//  always stands inside the shower without the system ever simulating rain on
//  the far side of the island.
// ---------------------------------------------------------------------------
class RainSystem {
public:
    RainSystem();

    void initialize(int capacity);
    void update(float deltaTime, const RenderContext& context, const Terrain& terrain);
    void render(const RenderContext& context) const;

    int aliveCount() const { return m_pool.aliveCount(); }
    int capacity()   const { return m_pool.capacity(); }

private:
    ParticlePool m_pool;
    util::Rng    m_rng;
    float m_spawnAccumulator;
};

// ---------------------------------------------------------------------------
//  Sparks rising off the campfire.
// ---------------------------------------------------------------------------
class SparkSystem {
public:
    SparkSystem();

    void initialize(int capacity);
    void setOrigin(const util::Vec3& origin) { m_origin = origin; }
    void update(float deltaTime, const RenderContext& context, float emissionRate);
    void render(const RenderContext& context) const;

    int aliveCount() const { return m_pool.aliveCount(); }

private:
    ParticlePool m_pool;
    util::Rng    m_rng;
    util::Vec3   m_origin;
    float m_spawnAccumulator;
};

// ---------------------------------------------------------------------------
//  Smoke drifting up off the campfire. Same pool machinery as SparkSystem -
//  deliberately not a new system, just a second consumer of ParticlePool
//  (section 22's "no duplicate systems" rule).
// ---------------------------------------------------------------------------
class SmokeSystem {
public:
    SmokeSystem();

    void initialize(int capacity);
    void setOrigin(const util::Vec3& origin) { m_origin = origin; }
    void update(float deltaTime, const RenderContext& context, float emissionRate);
    void render(const RenderContext& context) const;

    int aliveCount() const { return m_pool.aliveCount(); }

private:
    ParticlePool m_pool;
    util::Rng    m_rng;
    util::Vec3   m_origin;
    float m_spawnAccumulator;
};


// ---------------------------------------------------------------------------
// Sakura petals. A small fixed pool reuses the same particle storage as rain,
// sparks and smoke. Petals drift rather than falling straight down and remain
// concentrated around the hero Sakura tree.
// ---------------------------------------------------------------------------
class SakuraPetalSystem {
public:
    SakuraPetalSystem();

    void initialize(const util::Vec3& origin, unsigned seed, int capacity);
    void update(float deltaTime, const RenderContext& context, const Terrain& terrain);
    void render(const RenderContext& context) const;

    int aliveCount() const { return m_pool.aliveCount(); }
    int capacity() const { return m_pool.capacity(); }

private:
    ParticlePool m_pool;
    util::Rng m_rng;
    util::Vec3 m_origin;
    float m_spawnAccumulator;
};

// ---------------------------------------------------------------------------
//  Fireflies. Small emissive particles that drift around the treehouse,
//  campfire meadow and other flower-rich clearings. The pool is fixed-size,
//  just like rain and sparks, so the effect adds no per-frame allocations.
// ---------------------------------------------------------------------------
struct Firefly {
    util::Vec3 anchor;
    util::Vec3 position;
    float phase;
    float speed;
    float radius;
    float height;
    bool alive;

    Firefly()
        : anchor(), position(), phase(0.0f), speed(1.0f), radius(1.0f),
          height(0.8f), alive(false) {}
};

class FireflySystem {
public:
    FireflySystem();

    void initialize(const Terrain& terrain, unsigned seed, int count);
    void update(float deltaTime, const RenderContext& context);
    void render(const RenderContext& context) const;

    int aliveCount() const { return static_cast<int>(m_fireflies.size()); }
    int capacity() const { return static_cast<int>(m_fireflies.size()); }

private:
    std::vector<Firefly> m_fireflies;
    util::Rng m_rng;
    unsigned m_seed;
    float m_time;
};

#endif // LIVINGISLAND_PARTICLESYSTEM_H
