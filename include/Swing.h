// ============================================================================
// Swing.h - interactive swing, optionally hung from the Sakura hero branch.
// ============================================================================
#ifndef LIVINGISLAND_SWING_H
#define LIVINGISLAND_SWING_H

#include "GLIncludes.h"
#include "RenderContext.h"
#include "Utilities.h"

class Terrain;

class Swing {
public:
    Swing();

    // Legacy freestanding swing setup.
    void initialize(const Terrain& terrain, float x, float z, unsigned seed);
    // Tree-mounted setup: m_position is the ground point below the seat and
    // anchorHeight is measured above that terrain point.
    void initializeTreeMounted(const Terrain& terrain, float x, float z,
                               float anchorHeight, float facingDegrees,
                               unsigned seed);
    void buildRenderData();
    void destroyRenderData();

    void update(float deltaTime, bool occupied, float pumpInput);
    void renderSolid(const RenderContext& context) const;

    util::Vec3 position() const { return m_position; }
    util::Vec3 seatPosition() const;
    float baseFacing() const { return m_baseFacing; }
    bool treeMounted() const { return m_treeMounted; }

private:
    util::Vec3 m_position;
    float m_baseFacing;
    float m_angle;
    float m_angularVelocity;
    float m_ropeLength;
    float m_anchorHeight;
    GLuint m_frameList;
    bool m_treeMounted;
    bool m_built;
};

#endif // LIVINGISLAND_SWING_H
