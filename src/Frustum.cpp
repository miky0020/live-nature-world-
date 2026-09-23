#include "Frustum.h"
#include "GLIncludes.h"

Frustum::Frustum() {
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 4; ++j)
            m_planes[i][j] = 0.0f;
    // Degenerate until extracted: treat everything as visible.
    m_planes[0][3] = 1.0f;
}

static void normalizePlane(float plane[4]) {
    float length = std::sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
    if (length < 1e-8f) return;
    plane[0] /= length;
    plane[1] /= length;
    plane[2] /= length;
    plane[3] /= length;
}

void Frustum::extractFromCurrentMatrices() {
    GLfloat projection[16];
    GLfloat modelview[16];
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);

    // clip = projection * modelview (both column-major, as OpenGL stores them)
    GLfloat clip[16];
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            clip[column * 4 + row] =
                  modelview[column * 4 + 0] * projection[0 * 4 + row]
                + modelview[column * 4 + 1] * projection[1 * 4 + row]
                + modelview[column * 4 + 2] * projection[2 * 4 + row]
                + modelview[column * 4 + 3] * projection[3 * 4 + row];
        }
    }

    // Standard Gribb/Hartmann plane extraction.
    // right
    m_planes[0][0] = clip[3]  - clip[0];
    m_planes[0][1] = clip[7]  - clip[4];
    m_planes[0][2] = clip[11] - clip[8];
    m_planes[0][3] = clip[15] - clip[12];
    // left
    m_planes[1][0] = clip[3]  + clip[0];
    m_planes[1][1] = clip[7]  + clip[4];
    m_planes[1][2] = clip[11] + clip[8];
    m_planes[1][3] = clip[15] + clip[12];
    // bottom
    m_planes[2][0] = clip[3]  + clip[1];
    m_planes[2][1] = clip[7]  + clip[5];
    m_planes[2][2] = clip[11] + clip[9];
    m_planes[2][3] = clip[15] + clip[13];
    // top
    m_planes[3][0] = clip[3]  - clip[1];
    m_planes[3][1] = clip[7]  - clip[5];
    m_planes[3][2] = clip[11] - clip[9];
    m_planes[3][3] = clip[15] - clip[13];
    // far
    m_planes[4][0] = clip[3]  - clip[2];
    m_planes[4][1] = clip[7]  - clip[6];
    m_planes[4][2] = clip[11] - clip[10];
    m_planes[4][3] = clip[15] - clip[14];
    // near
    m_planes[5][0] = clip[3]  + clip[2];
    m_planes[5][1] = clip[7]  + clip[6];
    m_planes[5][2] = clip[11] + clip[10];
    m_planes[5][3] = clip[15] + clip[14];

    for (int i = 0; i < 6; ++i) normalizePlane(m_planes[i]);
}

bool Frustum::sphereVisible(const util::Vec3& center, float radius) const {
    for (int i = 0; i < 6; ++i) {
        float distance = m_planes[i][0] * center.x
                       + m_planes[i][1] * center.y
                       + m_planes[i][2] * center.z
                       + m_planes[i][3];
        if (distance < -radius) return false;   // entirely outside this plane
    }
    return true;
}
