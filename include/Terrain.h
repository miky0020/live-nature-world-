// ============================================================================
//  Terrain.h - procedural island heightfield.
//
//  The heightfield is the foundation the whole world sits on: every tree,
//  rock, character and structure added in later stages queries heightAt() so
//  nothing ever floats or sinks.
//
//  Geometry is baked once into a display list. It never changes at runtime, so
//  rebuilding it per frame would be pure waste.
// ============================================================================
#ifndef LIVINGISLAND_TERRAIN_H
#define LIVINGISLAND_TERRAIN_H

#include "GLIncludes.h"
#include "Utilities.h"

#include <vector>

class Terrain {
public:
    Terrain();
    ~Terrain();

    void generate(unsigned seed);
    void buildRenderData();          // must run after an OpenGL context exists
    void render() const;
    void destroyRenderData();

    // Bilinear sample of the same data the mesh was built from, so queries and
    // visible geometry agree exactly.
    float      heightAt(float x, float z) const;
    util::Vec3 normalAt(float x, float z) const;

    bool  isInsideWorld(float x, float z) const;

    // How strongly a worn path passes through this spot, 0..1. Used both to
    // tint the ground and to keep scattered objects off the walkways, so the
    // two can never disagree.
    static float pathInfluence(float x, float z);
    float halfSize()  const { return m_size * 0.5f; }
    float size()      const { return m_size; }
    int   resolution() const { return m_resolution; }

private:
    float rawHeight(int ix, int iz) const;
    static float computeHeight(float x, float z, unsigned seed);
    static util::Vec3 computeColor(float height, float slope,
                                   float x, float z, unsigned seed);

    int   m_resolution;
    float m_size;
    float m_cellSize;
    unsigned m_seed;

    std::vector<float>      m_heights;
    std::vector<util::Vec3> m_normals;
    std::vector<util::Vec3> m_colors;

    GLuint m_displayList;
};

#endif // LIVINGISLAND_TERRAIN_H
