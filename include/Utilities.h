// ============================================================================
//  Utilities.h - shared maths, deterministic RNG, value noise, timing, logging.
//  No OpenGL dependency, so this stays trivially testable.
// ============================================================================
#ifndef LIVINGISLAND_UTILITIES_H
#define LIVINGISLAND_UTILITIES_H

#include <cmath>
#include <string>

namespace util {

const float PI      = 3.14159265358979323846f;
const float TWO_PI  = 6.28318530717958647692f;

inline float toRadians(float degrees) { return degrees * (PI / 180.0f); }
inline float toDegrees(float radians) { return radians * (180.0f / PI); }

inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

inline float smoothstepf(float edge0, float edge1, float x) {
    if (edge1 <= edge0) return x < edge0 ? 0.0f : 1.0f;
    float t = clampf((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// Frame-rate independent smoothing. Naive `lerp(cur, target, dt * k)` changes
// behaviour with frame rate; this does not.
inline float damp(float current, float target, float rate, float dt) {
    return lerpf(target, current, std::exp(-rate * dt));
}

// Wrap a value into [0, range)
inline float wrapf(float v, float range) {
    if (range <= 0.0f) return 0.0f;
    float r = std::fmod(v, range);
    return r < 0.0f ? r + range : r;
}

// ---------------------------------------------------------------- Vec3 -----
struct Vec3 {
    float x, y, z;

    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}

    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator*(float s)       const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator/(float s)       const { return Vec3(x / s, y / s, z / s); }
    Vec3 operator-()              const { return Vec3(-x, -y, -z); }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s)       { x *= s;   y *= s;   z *= s;   return *this; }

    float lengthSquared() const { return x * x + y * y + z * z; }
    float length()        const { return std::sqrt(lengthSquared()); }

    Vec3 normalized() const {
        float len = length();
        return len > 1e-6f ? Vec3(x / len, y / len, z / len) : Vec3(0.0f, 0.0f, 0.0f);
    }
};

inline float dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}

inline Vec3 lerpv(const Vec3& a, const Vec3& b, float t) {
    return Vec3(lerpf(a.x, b.x, t), lerpf(a.y, b.y, t), lerpf(a.z, b.z, t));
}

inline float distanceXZ(const Vec3& a, const Vec3& b) {
    float dx = a.x - b.x, dz = a.z - b.z;
    return std::sqrt(dx * dx + dz * dz);
}

// A circular keep-out area. Scatterers consult these so trees never grow
// through the treehouse and rocks never sit in the campfire.
struct Zone {
    Vec3  center;
    float radius;
    Zone() : radius(0.0f) {}
    Zone(const Vec3& c, float r) : center(c), radius(r) {}
    bool contains(float x, float z) const {
        float dx = x - center.x, dz = z - center.z;
        return (dx * dx + dz * dz) < (radius * radius);
    }
};

// Shortest distance from a point to a line segment, in the XZ plane. Used for
// path proximity tests.
float distanceToSegmentXZ(float px, float pz,
                          float ax, float az,
                          float bx, float bz);

// ----------------------------------------------------------------- Rng -----
// Deterministic xorshift32. std::rand() differs between compilers and would
// give a different island on every machine, which is exactly what we do not
// want for a project that has to look the same during a demo.
class Rng {
public:
    explicit Rng(unsigned seed = 1u);
    void     reseed(unsigned seed);
    unsigned nextUInt();
    float    nextFloat();                   // [0, 1)
    float    range(float lo, float hi);
    int      rangeInt(int lo, int hi);       // inclusive
    bool     chance(float probability);
private:
    unsigned m_state;
};

// --------------------------------------------------------------- noise -----
float valueNoise2D(float x, float y, unsigned seed);
float fbm2D(float x, float y, int octaves, unsigned seed);

// --------------------------------------------------------------- clock -----
class Clock {
public:
    Clock();
    float tick();                 // seconds since previous tick, clamped
    float elapsed() const;        // seconds since construction
    void  reset();
private:
    double m_start;
    double m_last;
};

// ------------------------------------------------------------- logging -----
void logInfo(const std::string& message);
void logWarning(const std::string& message);
void logError(const std::string& message);

} // namespace util

#endif // LIVINGISLAND_UTILITIES_H
