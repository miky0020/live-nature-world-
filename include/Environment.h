// ============================================================================
//  Environment.h - the treehouse landmark and the smaller built props.
//
//  All the static woodwork is baked into one display list. The windows and the
//  lantern are drawn separately afterwards, because their colour has to change
//  with time of day - a lit window at midnight is what makes the treehouse
//  read as inhabited rather than abandoned.
// ============================================================================
#ifndef LIVINGISLAND_ENVIRONMENT_H
#define LIVINGISLAND_ENVIRONMENT_H

#include "GLIncludes.h"
#include "RenderContext.h"
#include "Utilities.h"

#include <vector>

class Terrain;

class Environment {
public:
    Environment();
    ~Environment();

    void initialize(const Terrain& terrain, unsigned seed);
    void buildRenderData();
    void destroyRenderData();
    void update(float deltaTime, const RenderContext& context);

    void renderSolid(const RenderContext& context) const;
    void renderEmissive(const RenderContext& context) const;

    void applyLanternLight(GLenum light, float nightFactor) const;

    util::Vec3 treehousePosition() const { return m_treehousePosition; }
    util::Vec3 lanternPosition()   const { return m_lanternPosition; }
    float platformHeight() const { return m_platformHeight; }

private:
    void buildTreehouse();
    void buildProps();

    util::Vec3 m_treehousePosition;
    util::Vec3 m_lanternPosition;
    float m_platformHeight;
    float m_lanternFlicker;
    float m_time;

    // Benches and logs around the campfire, placed once at init.
    struct Prop {
        util::Vec3 position;
        util::Vec3 color;
        float rotation;
        float scale;
        int   type;      // 0 = bench, 1 = log seat, 2 = post lantern
    };
    std::vector<Prop> m_props;

    GLuint m_treehouseList;
    GLuint m_benchList;
    GLuint m_logSeatList;
    unsigned m_seed;
    bool m_built;
};

#endif // LIVINGISLAND_ENVIRONMENT_H
