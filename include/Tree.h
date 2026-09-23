// ============================================================================
//  Tree.h - three tree species, scattered and animated.
//
//  The geometry trick that makes this cheap: a *unit* tree of each species and
//  detail level is compiled once into a display list, with no colour commands
//  inside. Each of the ~90 instances then just pushes a matrix, sets its own
//  colour and calls the list.
//
//  Trunk and foliage are separate lists so the canopy can be rotated by the
//  wind while the trunk stays planted - which is what section 7 asks for.
// ============================================================================
#ifndef LIVINGISLAND_TREE_H
#define LIVINGISLAND_TREE_H

#include "GLIncludes.h"
#include "RenderContext.h"
#include "Utilities.h"

#include <vector>

class Terrain;

enum TreeKind {
    TREE_LEAFY = 0,     // broad canopy, the visual backbone of the island
    TREE_TALL,          // tall and thin, breaks up the skyline
    TREE_SMALL,         // low decorative shrub-tree
    TREE_KIND_COUNT
};

const int TREE_LOD_COUNT = 3;   // 0 = full, 1 = simplified, 2 = crude

struct TreeInstance {
    util::Vec3 position;
    util::Vec3 trunkColor;
    util::Vec3 foliageColor;
    float heightScale;
    float widthScale;
    float rotation;      // degrees about Y
    float swayPhase;     // radians - the reason no two trees move alike
    float swaySpeed;
    float swayAmount;    // degrees at full wind
    float cullRadius;
    TreeKind kind;
};

class Forest {
public:
    Forest();
    ~Forest();

    void generate(const Terrain& terrain, unsigned seed,
                  const std::vector<util::Zone>& exclusions);
    void buildRenderData();
    void destroyRenderData();
    void render(const RenderContext& context) const;

    int count() const { return static_cast<int>(m_trees.size()); }
    const std::vector<TreeInstance>& trees() const { return m_trees; }

private:
    void buildSpeciesLists();
    static void buildTrunkGeometry(TreeKind kind, int lod);
    static void buildFoliageGeometry(TreeKind kind, int lod);
    static float canopyBaseHeight(TreeKind kind);

    std::vector<TreeInstance> m_trees;
    GLuint m_trunkLists[TREE_KIND_COUNT][TREE_LOD_COUNT];
    GLuint m_foliageLists[TREE_KIND_COUNT][TREE_LOD_COUNT];
    bool   m_listsBuilt;
};

#endif // LIVINGISLAND_TREE_H
