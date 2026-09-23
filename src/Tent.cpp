// ============================================================================
// Tent.cpp - lightweight procedural A-frame campsite tent.
// ============================================================================
#include "Tent.h"
#include "Primitives.h"
#include "Terrain.h"

using util::Vec3;

namespace {

// Draw one roof/fabric rectangle between four world-local corners.
void drawPanel(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d,
               const Vec3& color)
{
    Vec3 n = util::cross(b - a, d - a).normalized();
    glColor3f(color.x, color.y, color.z);
    glBegin(GL_QUADS);
        glNormal3f(n.x, n.y, n.z);
        glVertex3f(a.x, a.y, a.z);
        glVertex3f(b.x, b.y, b.z);
        glVertex3f(c.x, c.y, c.z);
        glVertex3f(d.x, d.y, d.z);
    glEnd();
}

void drawTriangle(const Vec3& a, const Vec3& b, const Vec3& c,
                  const Vec3& color)
{
    Vec3 n = util::cross(b - a, c - a).normalized();
    glColor3f(color.x, color.y, color.z);
    glBegin(GL_TRIANGLES);
        glNormal3f(n.x, n.y, n.z);
        glVertex3f(a.x, a.y, a.z);
        glVertex3f(b.x, b.y, b.z);
        glVertex3f(c.x, c.y, c.z);
    glEnd();
}

void drawPole(const Vec3& base, const Vec3& top, float radius, const Vec3& color)
{
    Vec3 delta = top - base;
    float length = delta.length();
    if (length <= 1e-4f) return;

    // The pole is a local +Y cylinder rotated onto the requested direction.
    Vec3 dir = delta.normalized();
    float angle = std::acos(util::clampf(dir.y, -1.0f, 1.0f)) * 180.0f / util::PI;
    Vec3 axis(-dir.z, 0.0f, dir.x);

    glPushMatrix();
    glTranslatef(base.x, base.y, base.z);
    if (axis.length() > 1e-5f) {
        axis = axis.normalized();
        glRotatef(angle, axis.x, axis.y, axis.z);
    }
    glColor3f(color.x, color.y, color.z);
    prim::drawCylinder(radius, radius * 0.90f, length, 6, false, false);
    glPopMatrix();
}

} // namespace

Tent::Tent()
    : m_position(0.0f, 0.0f, 0.0f)
    , m_renderList(0)
    , m_built(false)
{
}

void Tent::initialize(const Terrain& terrain, float x, float z) {
    m_position = Vec3(x, terrain.heightAt(x, z), z);
}

void Tent::buildRenderData() {
    if (m_built) return;

    m_renderList = glGenLists(1);
    if (!m_renderList) {
        util::logWarning("Could not allocate the camping tent display list.");
        return;
    }

    glNewList(m_renderList, GL_COMPILE);

    // Compact A-frame proportions based on the supplied campsite reference.
    // Ridge runs front-to-back; the entrance faces the campfire.
    const float halfWidth = 2.15f;
    const float halfDepth = 1.55f;
    const float wallHeight = 0.08f;
    const float ridgeHeight = 2.35f;
    const float frontZ = -halfDepth;
    const float backZ  =  halfDepth;

    const Vec3 canvas(0.76f, 0.30f, 0.075f);
    const Vec3 canvasLight(0.88f, 0.40f, 0.10f);
    const Vec3 canvasDark(0.55f, 0.20f, 0.055f);
    const Vec3 poleColor(0.22f, 0.13f, 0.07f);

    // Two sloping roof halves.
    drawPanel(Vec3(-halfWidth, wallHeight, frontZ),
              Vec3(-halfWidth, wallHeight, backZ),
              Vec3(0.0f, ridgeHeight, backZ),
              Vec3(0.0f, ridgeHeight, frontZ), canvas);
    drawPanel(Vec3(0.0f, ridgeHeight, frontZ),
              Vec3(0.0f, ridgeHeight, backZ),
              Vec3(halfWidth, wallHeight, backZ),
              Vec3(halfWidth, wallHeight, frontZ), canvasLight);

    // Gable ends keep the tent visually closed while preserving a dark front
    // entrance flap like the reference.
    drawTriangle(Vec3(-halfWidth, wallHeight, frontZ),
                 Vec3(0.0f, ridgeHeight, frontZ),
                 Vec3(halfWidth, wallHeight, frontZ), canvasDark);
    drawTriangle(Vec3(-halfWidth, wallHeight, backZ),
                 Vec3(halfWidth, wallHeight, backZ),
                 Vec3(0.0f, ridgeHeight, backZ), canvas);

    // Dark triangular doorway inset slightly toward the campfire side.
    const float doorHalf = 0.78f;
    const float doorTop = 1.62f;
    drawTriangle(Vec3(-doorHalf, wallHeight + 0.015f, frontZ - 0.012f),
                 Vec3( doorHalf, wallHeight + 0.015f, frontZ - 0.012f),
                 Vec3(0.0f, doorTop, frontZ - 0.012f),
                 Vec3(0.16f, 0.075f, 0.035f));

    // Four corner stakes/poles and the ridge support poles are intentionally
    // subtle so the canvas remains the visual focus.
    drawPole(Vec3(-halfWidth, 0.02f, frontZ), Vec3(0.0f, ridgeHeight, frontZ),
             0.035f, poleColor);
    drawPole(Vec3( halfWidth, 0.02f, frontZ), Vec3(0.0f, ridgeHeight, frontZ),
             0.035f, poleColor);
    drawPole(Vec3(-halfWidth, 0.02f, backZ), Vec3(0.0f, ridgeHeight, backZ),
             0.035f, poleColor);
    drawPole(Vec3( halfWidth, 0.02f, backZ), Vec3(0.0f, ridgeHeight, backZ),
             0.035f, poleColor);

    // Ridge line and two small guy-line anchors add the handmade camping look.
    glColor3f(0.33f, 0.19f, 0.09f);
    glPushMatrix();
    glTranslatef(0.0f, ridgeHeight, 0.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    prim::drawCylinder(0.045f, 0.038f, halfDepth * 2.0f, 6, false, false);
    glPopMatrix();

    glColor3f(0.55f, 0.34f, 0.16f);
    glBegin(GL_LINES);
        glVertex3f(-halfWidth, wallHeight, frontZ);
        glVertex3f(-halfWidth - 0.35f, 0.03f, frontZ - 0.42f);
        glVertex3f( halfWidth, wallHeight, frontZ);
        glVertex3f( halfWidth + 0.35f, 0.03f, frontZ - 0.42f);
    glEnd();

    // A small rolled entrance flap at the base gives the front more depth.
    glColor3f(0.63f, 0.25f, 0.06f);
    glPushMatrix();
    glTranslatef(0.0f, 0.12f, frontZ - 0.035f);
    glScalef(0.82f, 0.08f, 0.08f);
    prim::drawBlob(1.0f, 7, 4, 431u, 0.12f, 0.70f);
    glPopMatrix();

    glEndList();
    m_built = true;
    util::logInfo("Camping tent geometry compiled.");
}

void Tent::destroyRenderData() {
    if (m_renderList) glDeleteLists(m_renderList, 1);
    m_renderList = 0;
    m_built = false;
}

void Tent::render(const RenderContext& context) const {
    if (!m_built) return;
    if (!context.frustum.sphereVisible(m_position + Vec3(0.0f, 1.0f, 0.0f), 3.4f)) return;

    ++context.visibleProps;
    glPushMatrix();
    glTranslatef(m_position.x, m_position.y, m_position.z);
    glCallList(m_renderList);
    glPopMatrix();
}
