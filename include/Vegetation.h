// ============================================================================
//  Vegetation.h - grass clusters, flowers and rocks.
//
//  Section 6 warns against thousands of individual expensive objects, so the
//  approach is the same one the forest uses: a handful of shared display lists
//  for the geometry, and a lightweight instance record per placement holding
//  only what varies. A grass cluster costs a matrix push, a colour and a list
//  call - no geometry is generated at runtime.
//
//  Grass also gets an aggressive distance cull: a tuft two hundred units away
//  covers a fraction of a pixel, so drawing it is pure waste.
// ============================================================================
#ifndef LIVINGISLAND_VEGETATION_H
#define LIVINGISLAND_VEGETATION_H

#include "GLIncludes.h"
#include "RenderContext.h"
#include "Utilities.h"

#include <vector>

class Terrain;

const int ROCK_VARIANT_COUNT = 4;

class Vegetation {
public:
    Vegetation();
    ~Vegetation();

    void generate(const Terrain& terrain, unsigned seed,
                  const std::vector<util::Zone>& exclusions);
    void buildRenderData();
    void destroyRenderData();

    void renderRocks(const RenderContext& context) const;
    void renderGrassAndFlowers(const RenderContext& context) const;

    int grassCount()  const { return static_cast<int>(m_grass.size()); }
    int flowerCount() const { return static_cast<int>(m_flowers.size()); }
    int rockCount()   const { return static_cast<int>(m_rocks.size()); }

private:
    struct GrassInstance {
        util::Vec3 position;
        util::Vec3 color;
        float scale;
        float rotation;
        float phase;
    };

    struct FlowerInstance {
        util::Vec3 position;
        util::Vec3 color;
        float scale;
        float rotation;
        float phase;
    };

    struct RockInstance {
        util::Vec3 position;
        util::Vec3 color;
        float scale;
        float rotation;
        float tilt;
        int   variant;
    };

    // Spatial index. Without it, every frame walks all 5000 grass instances
    // and does a frustum test on each - pure CPU cost for objects that are
    // mostly behind the camera. The grid lets a single sphere test reject a
    // whole cell of tufts at once, so the cost scales with what is actually
    // on screen rather than with the size of the island.
    struct GridCell {
        std::vector<int> grass;
        std::vector<int> flowers;
        util::Vec3 center;
        float radius;
        GridCell() : radius(0.0f) {}
    };

    void buildSpatialIndex();
    int  cellIndex(float x, float z) const;

    std::vector<GridCell> m_cells;
    int   m_gridRes;
    float m_gridCellSize;
    float m_gridOrigin;

    std::vector<GrassInstance>  m_grass;
    std::vector<FlowerInstance> m_flowers;
    std::vector<RockInstance>   m_rocks;

    GLuint m_grassList;
    GLuint m_grassMidList;
    GLuint m_grassFarList;
    GLuint m_flowerList;
    GLuint m_rockLists[ROCK_VARIANT_COUNT];
    bool   m_listsBuilt;

    float m_grassDrawDistance;
    float m_flowerDrawDistance;
    float m_grassNearDistance;   // full-detail tuft inside this
    float m_grassMidDistance;    // reduced tuft inside this, crude beyond
};

#endif // LIVINGISLAND_VEGETATION_H
