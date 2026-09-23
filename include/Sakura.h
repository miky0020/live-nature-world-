// ============================================================================
// Sakura.h - one hero cherry-blossom tree beside the campfire.
// ============================================================================
#ifndef LIVINGISLAND_SAKURA_H
#define LIVINGISLAND_SAKURA_H

#include "GLIncludes.h"
#include "RenderContext.h"
#include "Utilities.h"

class Terrain;

class SakuraTree {
public:
    SakuraTree();

    void initialize(const Terrain& terrain, float x, float z, unsigned seed);
    void buildRenderData();
    void destroyRenderData();
    void update(float deltaTime, const RenderContext& context);
    void render(const RenderContext& context) const;

    util::Vec3 position() const { return m_position; }
    // World-space point where the swing's two ropes meet the Sakura branch.
    util::Vec3 swingAnchor() const;
    // World-space direction along the strong branch, used to orient the seat.
    float swingFacing() const { return m_swingFacing; }

private:
    util::Vec3 m_position;
    unsigned m_seed;
    float m_time;
    float m_sway;
    float m_swingFacing;
    GLuint m_renderList;
    bool m_built;
};

#endif // LIVINGISLAND_SAKURA_H
