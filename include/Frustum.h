// ============================================================================
//  Frustum.h - view frustum extracted from the current GL matrices.
//
//  With ~90 trees, ~900 grass clusters and a few hundred props, most of the
//  world is behind the camera at any moment. Rejecting those with six dot
//  products each is far cheaper than pushing them through the pipeline.
// ============================================================================
#ifndef LIVINGISLAND_FRUSTUM_H
#define LIVINGISLAND_FRUSTUM_H

#include "Utilities.h"

class Frustum {
public:
    Frustum();

    // Reads GL_PROJECTION and GL_MODELVIEW, so it must be called after the
    // camera has applied both.
    void extractFromCurrentMatrices();

    bool sphereVisible(const util::Vec3& center, float radius) const;

private:
    float m_planes[6][4];   // a, b, c, d for each plane
};

#endif // LIVINGISLAND_FRUSTUM_H
