#include "Primitives.h"

namespace prim {

// Deterministic 0..1 hash used for blob jitter.
static float hashUnit(int a, int b, unsigned seed) {
    unsigned h = seed + 0x9E3779B9u;
    h ^= static_cast<unsigned>(a) * 0x85EBCA6Bu;
    h = (h << 13) | (h >> 19);
    h ^= static_cast<unsigned>(b) * 0xC2B2AE35u;
    h *= 0x27D4EB2Fu;
    h ^= h >> 15;
    return static_cast<float>(h & 0x00FFFFFFu) / static_cast<float>(0x00FFFFFFu);
}

void drawCylinder(float baseRadius, float topRadius, float height,
                  int slices, bool capBase, bool capTop)
{
    if (slices < 3) slices = 3;

    // The side normal has to account for the taper, otherwise a cone lights
    // like a cylinder and looks wrong.
    float slope = (baseRadius - topRadius);
    float normalScale = std::sqrt(height * height + slope * slope);
    float ny = normalScale > 1e-6f ? slope / normalScale : 0.0f;
    float nr = normalScale > 1e-6f ? height / normalScale : 1.0f;

    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= slices; ++i) {
        float angle = (static_cast<float>(i) / static_cast<float>(slices)) * util::TWO_PI;
        float c = std::cos(angle);
        float s = std::sin(angle);

        glNormal3f(c * nr, ny, s * nr);
        glVertex3f(c * baseRadius, 0.0f,   s * baseRadius);
        glVertex3f(c * topRadius,  height, s * topRadius);
    }
    glEnd();

    if (capBase && baseRadius > 1e-5f) {
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0f, -1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        for (int i = slices; i >= 0; --i) {
            float angle = (static_cast<float>(i) / static_cast<float>(slices)) * util::TWO_PI;
            glVertex3f(std::cos(angle) * baseRadius, 0.0f, std::sin(angle) * baseRadius);
        }
        glEnd();
    }

    if (capTop && topRadius > 1e-5f) {
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, height, 0.0f);
        for (int i = 0; i <= slices; ++i) {
            float angle = (static_cast<float>(i) / static_cast<float>(slices)) * util::TWO_PI;
            glVertex3f(std::cos(angle) * topRadius, height, std::sin(angle) * topRadius);
        }
        glEnd();
    }
}

void drawSphere(float radius, int slices, int stacks) {
    if (slices < 3) slices = 3;
    if (stacks < 2) stacks = 2;

    for (int stack = 0; stack < stacks; ++stack) {
        float phi0 = util::PI * (static_cast<float>(stack)     / static_cast<float>(stacks)) - util::PI * 0.5f;
        float phi1 = util::PI * (static_cast<float>(stack + 1) / static_cast<float>(stacks)) - util::PI * 0.5f;

        float y0 = std::sin(phi0), r0 = std::cos(phi0);
        float y1 = std::sin(phi1), r1 = std::cos(phi1);

        glBegin(GL_TRIANGLE_STRIP);
        for (int slice = 0; slice <= slices; ++slice) {
            float theta = (static_cast<float>(slice) / static_cast<float>(slices)) * util::TWO_PI;
            float c = std::cos(theta), s = std::sin(theta);

            glNormal3f(c * r0, y0, s * r0);
            glVertex3f(c * r0 * radius, y0 * radius, s * r0 * radius);

            glNormal3f(c * r1, y1, s * r1);
            glVertex3f(c * r1 * radius, y1 * radius, s * r1 * radius);
        }
        glEnd();
    }
}

void drawBlob(float radius, int slices, int stacks, unsigned seed,
              float jitter, float flatten)
{
    if (slices < 3) slices = 3;
    if (stacks < 2) stacks = 2;

    for (int stack = 0; stack < stacks; ++stack) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int slice = 0; slice <= slices; ++slice) {
            // Wrap the last column back onto the first so the seam closes.
            int sliceIndex = (slice == slices) ? 0 : slice;

            for (int row = 0; row < 2; ++row) {
                int stackIndex = stack + row;

                float phi   = util::PI * (static_cast<float>(stackIndex) / static_cast<float>(stacks))
                            - util::PI * 0.5f;
                float theta = (static_cast<float>(slice) / static_cast<float>(slices)) * util::TWO_PI;

                // Poles must not wobble sideways or the blob tears open.
                bool pole = (stackIndex == 0 || stackIndex == stacks);
                float wobble = pole ? hashUnit(0, stackIndex, seed)
                                    : hashUnit(sliceIndex, stackIndex, seed);
                float r = radius * (1.0f + (wobble - 0.5f) * 2.0f * jitter);

                float cy = std::sin(phi);
                float cr = std::cos(phi);
                float c  = std::cos(theta), s = std::sin(theta);

                util::Vec3 normal(c * cr, cy / (flatten > 1e-4f ? flatten : 1.0f), s * cr);
                normal = normal.normalized();

                glNormal3f(normal.x, normal.y, normal.z);
                glVertex3f(c * cr * r, cy * r * flatten, s * cr * r);
            }
        }
        glEnd();
    }
}

void drawBox(float sizeX, float sizeY, float sizeZ) {
    float x = sizeX * 0.5f, y = sizeY * 0.5f, z = sizeZ * 0.5f;

    glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(-x, -y,  z); glVertex3f( x, -y,  z);
        glVertex3f( x,  y,  z); glVertex3f(-x,  y,  z);

        glNormal3f(0.0f, 0.0f, -1.0f);
        glVertex3f( x, -y, -z); glVertex3f(-x, -y, -z);
        glVertex3f(-x,  y, -z); glVertex3f( x,  y, -z);

        glNormal3f(-1.0f, 0.0f, 0.0f);
        glVertex3f(-x, -y, -z); glVertex3f(-x, -y,  z);
        glVertex3f(-x,  y,  z); glVertex3f(-x,  y, -z);

        glNormal3f(1.0f, 0.0f, 0.0f);
        glVertex3f( x, -y,  z); glVertex3f( x, -y, -z);
        glVertex3f( x,  y, -z); glVertex3f( x,  y,  z);

        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(-x,  y,  z); glVertex3f( x,  y,  z);
        glVertex3f( x,  y, -z); glVertex3f(-x,  y, -z);

        glNormal3f(0.0f, -1.0f, 0.0f);
        glVertex3f(-x, -y, -z); glVertex3f( x, -y, -z);
        glVertex3f( x, -y,  z); glVertex3f(-x, -y,  z);
    glEnd();
}

void drawPyramid(float baseX, float baseZ, float height) {
    float x = baseX * 0.5f, z = baseZ * 0.5f;
    util::Vec3 apex(0.0f, height, 0.0f);

    util::Vec3 corners[4] = {
        util::Vec3(-x, 0.0f,  z), util::Vec3( x, 0.0f,  z),
        util::Vec3( x, 0.0f, -z), util::Vec3(-x, 0.0f, -z)
    };

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < 4; ++i) {
        const util::Vec3& a = corners[i];
        const util::Vec3& b = corners[(i + 1) % 4];
        util::Vec3 normal = util::cross(b - a, apex - a).normalized();
        glNormal3f(normal.x, normal.y, normal.z);
        glVertex3f(a.x, a.y, a.z);
        glVertex3f(b.x, b.y, b.z);
        glVertex3f(apex.x, apex.y, apex.z);
    }
    glEnd();

    glBegin(GL_QUADS);
        glNormal3f(0.0f, -1.0f, 0.0f);
        glVertex3f(-x, 0.0f, -z); glVertex3f( x, 0.0f, -z);
        glVertex3f( x, 0.0f,  z); glVertex3f(-x, 0.0f,  z);
    glEnd();
}

void drawQuadXZ(float sizeX, float sizeZ) {
    float x = sizeX * 0.5f, z = sizeZ * 0.5f;
    glBegin(GL_QUADS);
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(-x, 0.0f, -z); glVertex3f(-x, 0.0f,  z);
        glVertex3f( x, 0.0f,  z); glVertex3f( x, 0.0f, -z);
    glEnd();
}

void drawUprightQuad(float width, float height) {
    float w = width * 0.5f;

    // The normal points mostly UP rather than out along the quad's facing.
    // A physically "correct" outward normal makes every blade catch only
    // grazing light and read as a dark floating card; biasing it upward makes
    // the tuft light like the ground it grows from, which is what the eye
    // expects. Cheating the normal is standard practice for foliage.
    glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.93f, 0.37f);
        glVertex3f(-w, 0.0f, 0.0f);
        glVertex3f( w, 0.0f, 0.0f);
        // Taper toward the tip so it reads as a blade, not a rectangle.
        glVertex3f( w * 0.22f, height, 0.0f);
        glVertex3f(-w * 0.22f, height, 0.0f);
    glEnd();
}

void drawCrossQuads(float width, float height) {
    drawUprightQuad(width, height);
    glPushMatrix();
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    drawUprightQuad(width, height);
    glPopMatrix();
}

void drawGrassBlade(float width, float height, float bend, int segments) {
    if (segments < 2) segments = 2;

    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(segments);

        // Quadratic arc: the blade leans further the higher it goes, which is
        // how a real blade behaves under its own weight.
        float y = height * t;
        float z = bend * t * t;

        // Taper to a point at the tip.
        float w = width * (1.0f - t * t * 0.92f) * 0.5f;

        // Normal biased upward so the blade lights like the ground it grows
        // from; a true surface normal leaves it black at most sun angles.
        float slope = 2.0f * bend * t / (height > 1e-5f ? height : 1.0f);
        util::Vec3 n(0.0f, 1.0f, -slope * 0.35f);
        n = n.normalized();
        glNormal3f(n.x, n.y, n.z);

        glVertex3f(-w, y, z);
        glVertex3f( w, y, z);
    }
    glEnd();
}

void drawLeafSpray(float radius, float leafLength, int leafCount, unsigned seed) {
    if (leafCount < 1) leafCount = 1;

    for (int i = 0; i < leafCount; ++i) {
        float a1 = hashUnit(i, 1, seed);
        float a2 = hashUnit(i, 2, seed);
        float a3 = hashUnit(i, 3, seed);

        float yaw   = a1 * 360.0f;
        float pitch = 18.0f + a2 * 62.0f;       // droop away from vertical
        float scale = 0.65f + a3 * 0.55f;

        glPushMatrix();
        glRotatef(yaw, 0.0f, 1.0f, 0.0f);
        glTranslatef(radius * (0.25f + a2 * 0.55f), 0.0f, 0.0f);
        glRotatef(pitch, 0.0f, 0.0f, -1.0f);
        drawGrassBlade(leafLength * 0.52f * scale, leafLength * scale,
                       leafLength * 0.30f, 3);
        glPopMatrix();
    }
}

} // namespace prim
