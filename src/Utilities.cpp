#include "Utilities.h"
#include "Config.h"

#include <chrono>
#include <cstdio>

namespace util {

// ====================================================== geometry ============
float distanceToSegmentXZ(float px, float pz, float ax, float az, float bx, float bz) {
    float abx = bx - ax, abz = bz - az;
    float apx = px - ax, apz = pz - az;
    float lengthSq = abx * abx + abz * abz;
    float t = lengthSq > 1e-6f ? clampf((apx * abx + apz * abz) / lengthSq, 0.0f, 1.0f) : 0.0f;
    float cx = ax + abx * t, cz = az + abz * t;
    float dx = px - cx, dz = pz - cz;
    return std::sqrt(dx * dx + dz * dz);
}

// =========================================================== Rng ============
Rng::Rng(unsigned seed) : m_state(seed ? seed : 0x9E3779B9u) {}

void Rng::reseed(unsigned seed) { m_state = seed ? seed : 0x9E3779B9u; }

unsigned Rng::nextUInt() {
    // xorshift32 - fast, deterministic, more than good enough for scatter.
    m_state ^= m_state << 13;
    m_state ^= m_state >> 17;
    m_state ^= m_state << 5;
    return m_state;
}

float Rng::nextFloat() {
    return static_cast<float>(nextUInt() & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
}

float Rng::range(float lo, float hi) { return lo + (hi - lo) * nextFloat(); }

int Rng::rangeInt(int lo, int hi) {
    if (hi <= lo) return lo;
    return lo + static_cast<int>(nextUInt() % static_cast<unsigned>(hi - lo + 1));
}

bool Rng::chance(float probability) { return nextFloat() < probability; }

// ========================================================= noise ============
static unsigned hashInt2(int x, int y, unsigned seed) {
    unsigned h = seed + 0x9E3779B9u;
    h ^= static_cast<unsigned>(x) * 0x85EBCA6Bu;
    h = (h << 13) | (h >> 19);
    h ^= static_cast<unsigned>(y) * 0xC2B2AE35u;
    h *= 0x27D4EB2Fu;
    h ^= h >> 15;
    return h;
}

static float hashFloat2(int x, int y, unsigned seed) {
    return static_cast<float>(hashInt2(x, y, seed) & 0x00FFFFFFu)
         / static_cast<float>(0x00FFFFFFu);
}

float valueNoise2D(float x, float y, unsigned seed) {
    float fx = std::floor(x);
    float fy = std::floor(y);
    int   ix = static_cast<int>(fx);
    int   iy = static_cast<int>(fy);

    float tx = x - fx;
    float ty = y - fy;
    float sx = tx * tx * (3.0f - 2.0f * tx);
    float sy = ty * ty * (3.0f - 2.0f * ty);

    float a = hashFloat2(ix,     iy,     seed);
    float b = hashFloat2(ix + 1, iy,     seed);
    float c = hashFloat2(ix,     iy + 1, seed);
    float d = hashFloat2(ix + 1, iy + 1, seed);

    return lerpf(lerpf(a, b, sx), lerpf(c, d, sx), sy);
}

float fbm2D(float x, float y, int octaves, unsigned seed) {
    float sum = 0.0f, amplitude = 0.5f, frequency = 1.0f, norm = 0.0f;
    for (int i = 0; i < octaves; ++i) {
        sum  += valueNoise2D(x * frequency, y * frequency,
                             seed + static_cast<unsigned>(i) * 101u) * amplitude;
        norm += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return norm > 0.0f ? sum / norm : 0.0f;
}

// ========================================================= Clock ============
static double nowSeconds() {
    using namespace std::chrono;
    static const steady_clock::time_point origin = steady_clock::now();
    return duration_cast<duration<double> >(steady_clock::now() - origin).count();
}

Clock::Clock() { reset(); }

void Clock::reset() {
    m_start = nowSeconds();
    m_last  = m_start;
}

float Clock::tick() {
    double now = nowSeconds();
    double dt  = now - m_last;
    m_last = now;
    if (dt < 0.0) dt = 0.0;
    if (dt > static_cast<double>(Config::MAX_DELTA_TIME))
        dt = static_cast<double>(Config::MAX_DELTA_TIME);
    return static_cast<float>(dt);
}

float Clock::elapsed() const { return static_cast<float>(m_last - m_start); }

// ======================================================= logging ============
void logInfo(const std::string& message) {
    std::printf("[INFO] %s\n", message.c_str());
    std::fflush(stdout);
}

void logWarning(const std::string& message) {
    std::printf("[WARNING] %s\n", message.c_str());
    std::fflush(stdout);
}

void logError(const std::string& message) {
    std::fprintf(stderr, "[ERROR] %s\n", message.c_str());
    std::fflush(stderr);
}

} // namespace util
