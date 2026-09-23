// ============================================================================
// Tent.h - a small A-frame camping tent beside the hero campfire.
// ============================================================================
#ifndef LIVINGISLAND_TENT_H
#define LIVINGISLAND_TENT_H

#include "GLIncludes.h"
#include "RenderContext.h"
#include "Utilities.h"

class Terrain;

class Tent {
public:
    Tent();

    void initialize(const Terrain& terrain, float x, float z);
    void buildRenderData();
    void destroyRenderData();
    void render(const RenderContext& context) const;

    util::Vec3 position() const { return m_position; }

private:
    util::Vec3 m_position;
    GLuint m_renderList;
    bool m_built;
};

#endif // LIVINGISLAND_TENT_H
