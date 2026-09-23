#include "Tree.h"
#include "Config.h"
#include "Primitives.h"
#include "Terrain.h"

#include <cstdio>

using util::Vec3;

Forest::Forest() : m_listsBuilt(false) {
    for (int k = 0; k < TREE_KIND_COUNT; ++k)
        for (int l = 0; l < TREE_LOD_COUNT; ++l) {
            m_trunkLists[k][l] = 0;
            m_foliageLists[k][l] = 0;
        }
}

Forest::~Forest() {}

// Height at which the canopy starts, as a fraction of total tree height. The
// sway rotation pivots here so the trunk stays rooted.
float Forest::canopyBaseHeight(TreeKind kind) {
    switch (kind) {
        case TREE_LEAFY: return 2.6f;
        case TREE_TALL:  return 4.6f;
        case TREE_SMALL: return 1.1f;
        default:         return 2.0f;
    }
}

// ---------------------------------------------------------------------------
//  Geometry. Each species is built at unit scale; instances scale it.
// ---------------------------------------------------------------------------
void Forest::buildTrunkGeometry(TreeKind kind, int lod) {
    int slices = (lod == 0) ? 10 : (lod == 1 ? 7 : 5);

    switch (kind) {
        case TREE_LEAFY:
            // Trunk in two tapered sections, so it narrows the way a real one
            // does instead of being a straight tube.
            prim::drawCylinder(0.36f, 0.26f, 1.7f, slices, false, false);
            glPushMatrix();
            glTranslatef(0.0f, 1.7f, 0.0f);
            prim::drawCylinder(0.26f, 0.17f, 1.4f, slices, false, false);
            glPopMatrix();

            if (lod == 0) {
                // Four branches reaching up into the canopy.
                for (int b = 0; b < 4; ++b) {
                    float yaw = 42.0f + static_cast<float>(b) * 88.0f;
                    float lift = 1.85f + static_cast<float>(b % 2) * 0.42f;
                    glPushMatrix();
                    glRotatef(yaw, 0.0f, 1.0f, 0.0f);
                    glTranslatef(0.0f, lift, 0.0f);
                    glRotatef(46.0f, 0.0f, 0.0f, 1.0f);
                    prim::drawCylinder(0.115f, 0.045f, 1.35f, 7, false, false);
                    glPopMatrix();
                }
            }
            break;

        case TREE_TALL:
            prim::drawCylinder(0.24f, 0.17f, 2.9f, slices, false, false);
            glPushMatrix();
            glTranslatef(0.0f, 2.9f, 0.0f);
            prim::drawCylinder(0.17f, 0.11f, 2.4f, slices, false, false);
            glPopMatrix();
            break;

        case TREE_SMALL:
            prim::drawCylinder(0.20f, 0.14f, 1.3f, slices, false, false);
            break;

        default: break;
    }
}

void Forest::buildFoliageGeometry(TreeKind kind, int lod) {
    // Canopies are built from two layers:
    //   1. rounded masses, at a far higher tessellation than before, which
    //      give the tree its overall volume and shading
    //   2. sprays of individual tapered leaves poking out of those masses,
    //      which break the smooth silhouette that made the old trees read as
    //      green balloons
    // The leaf sprays are dropped entirely at lower LODs, where they would be
    // sub-pixel anyway.
    switch (kind) {
        case TREE_LEAFY: {
            if (lod == 2) {
                glPushMatrix();
                glTranslatef(0.0f, 1.0f, 0.0f);
                prim::drawBlob(1.5f, 8, 6, 11u, 0.12f, 0.85f);
                glPopMatrix();
                break;
            }
            int slices = (lod == 0) ? 11 : 8;
            int stacks = (lod == 0) ? 8  : 5;

            // Five overlapping masses read as a canopy rather than a ball.
            glPushMatrix();
            glTranslatef(0.0f, 1.15f, 0.0f);
            prim::drawBlob(1.45f, slices, stacks, 11u, 0.24f, 0.82f);
            glPopMatrix();

            glPushMatrix();
            glTranslatef(0.78f, 0.55f, 0.24f);
            prim::drawBlob(0.92f, slices, stacks, 23u, 0.26f, 0.86f);
            glPopMatrix();

            glPushMatrix();
            glTranslatef(-0.66f, 0.62f, -0.42f);
            prim::drawBlob(0.86f, slices, stacks, 37u, 0.26f, 0.86f);
            glPopMatrix();

            glPushMatrix();
            glTranslatef(0.18f, 0.70f, -0.78f);
            prim::drawBlob(0.74f, slices, stacks, 41u, 0.26f, 0.88f);
            glPopMatrix();

            glPushMatrix();
            glTranslatef(0.12f, 1.95f, -0.18f);
            prim::drawBlob(0.72f, slices, stacks, 53u, 0.28f, 0.90f);
            glPopMatrix();

            if (lod == 0) {
                // Leaf detail around the outside of the canopy.
                glPushMatrix();
                glTranslatef(0.0f, 1.45f, 0.0f);
                prim::drawLeafSpray(1.55f, 0.56f, 14, 611u);
                glPopMatrix();

                glPushMatrix();
                glTranslatef(0.0f, 0.75f, 0.0f);
                prim::drawLeafSpray(1.35f, 0.48f, 10, 617u);
                glPopMatrix();

                glPushMatrix();
                glTranslatef(0.10f, 2.10f, -0.15f);
                prim::drawLeafSpray(0.80f, 0.42f, 7, 619u);
                glPopMatrix();
            }
            break;
        }

        case TREE_TALL: {
            if (lod == 2) {
                glPushMatrix();
                glTranslatef(0.0f, 0.9f, 0.0f);
                prim::drawBlob(0.85f, 8, 6, 61u, 0.12f, 1.35f);
                glPopMatrix();
                break;
            }
            int slices = (lod == 0) ? 10 : 7;
            int stacks = (lod == 0) ? 7  : 5;

            // Stacked narrowing tufts give this species its own silhouette.
            glPushMatrix();
            glTranslatef(0.0f, 0.35f, 0.0f);
            prim::drawBlob(0.86f, slices, stacks, 61u, 0.24f, 1.15f);
            glPopMatrix();

            glPushMatrix();
            glTranslatef(0.10f, 1.25f, 0.06f);
            prim::drawBlob(0.70f, slices, stacks, 73u, 0.26f, 1.20f);
            glPopMatrix();

            glPushMatrix();
            glTranslatef(-0.08f, 2.05f, -0.05f);
            prim::drawBlob(0.50f, slices, stacks, 89u, 0.28f, 1.25f);
            glPopMatrix();

            if (lod == 0) {
                glPushMatrix();
                glTranslatef(0.0f, 0.55f, 0.0f);
                prim::drawLeafSpray(0.95f, 0.66f, 10, 701u);
                glPopMatrix();

                glPushMatrix();
                glTranslatef(0.08f, 1.50f, 0.04f);
                prim::drawLeafSpray(0.78f, 0.58f, 8, 709u);
                glPopMatrix();

                glPushMatrix();
                glTranslatef(-0.06f, 2.30f, -0.04f);
                prim::drawLeafSpray(0.55f, 0.46f, 6, 719u);
                glPopMatrix();
            }
            break;
        }

        case TREE_SMALL: {
            if (lod == 2) {
                glPushMatrix();
                glTranslatef(0.0f, 0.45f, 0.0f);
                prim::drawBlob(0.78f, 8, 5, 97u, 0.14f, 0.80f);
                glPopMatrix();
                break;
            }
            int slices = (lod == 0) ? 10 : 7;
            int stacks = (lod == 0) ? 7  : 5;

            glPushMatrix();
            glTranslatef(0.0f, 0.5f, 0.0f);
            prim::drawBlob(0.80f, slices, stacks, 97u, 0.30f, 0.78f);
            glPopMatrix();

            glPushMatrix();
            glTranslatef(0.42f, 0.16f, 0.30f);
            prim::drawBlob(0.50f, slices, stacks, 103u, 0.30f, 0.80f);
            glPopMatrix();

            glPushMatrix();
            glTranslatef(-0.38f, 0.22f, -0.26f);
            prim::drawBlob(0.46f, slices, stacks, 109u, 0.30f, 0.80f);
            glPopMatrix();

            if (lod == 0) {
                glPushMatrix();
                glTranslatef(0.0f, 0.62f, 0.0f);
                prim::drawLeafSpray(0.90f, 0.44f, 10, 811u);
                glPopMatrix();
            }
            break;
        }

        default: break;
    }
}

void Forest::buildSpeciesLists() {
    for (int kind = 0; kind < TREE_KIND_COUNT; ++kind) {
        for (int lod = 0; lod < TREE_LOD_COUNT; ++lod) {
            GLuint trunk = glGenLists(1);
            if (trunk) {
                glNewList(trunk, GL_COMPILE);
                buildTrunkGeometry(static_cast<TreeKind>(kind), lod);
                glEndList();
            }
            m_trunkLists[kind][lod] = trunk;

            GLuint foliage = glGenLists(1);
            if (foliage) {
                glNewList(foliage, GL_COMPILE);
                buildFoliageGeometry(static_cast<TreeKind>(kind), lod);
                glEndList();
            }
            m_foliageLists[kind][lod] = foliage;
        }
    }
    m_listsBuilt = true;
}

// ---------------------------------------------------------------------------
//  Placement
// ---------------------------------------------------------------------------
void Forest::generate(const Terrain& terrain, unsigned seed,
                      const std::vector<util::Zone>& exclusions)
{
    m_trees.clear();
    m_trees.reserve(static_cast<size_t>(Config::TREE_COUNT));

    util::Rng rng(seed ^ 0x77EEu);
    const float half = terrain.halfSize();

    int attempts = 0;
    const int maxAttempts = Config::TREE_COUNT * 60;

    while (static_cast<int>(m_trees.size()) < Config::TREE_COUNT && attempts < maxAttempts) {
        ++attempts;

        float x = rng.range(-half + 8.0f, half - 8.0f);
        float z = rng.range(-half + 8.0f, half - 8.0f);

        float ground = terrain.heightAt(x, z);

        // No trees in the sea, on the beach, or on cliff faces.
        if (ground < Config::WATER_LEVEL + 1.2f) continue;
        Vec3 normal = terrain.normalAt(x, z);
        if (normal.y < 0.80f) continue;

        // Keep the landmarks and the paths clear (section 27).
        bool blocked = false;
        for (size_t i = 0; i < exclusions.size(); ++i) {
            if (exclusions[i].contains(x, z)) { blocked = true; break; }
        }
        if (blocked) continue;
        if (Terrain::pathInfluence(x, z) > 0.05f) continue;

        // Minimum spacing, so the forest does not clump into solid walls.
        bool tooClose = false;
        for (size_t i = 0; i < m_trees.size(); ++i) {
            float dx = m_trees[i].position.x - x;
            float dz = m_trees[i].position.z - z;
            if (dx * dx + dz * dz < 36.0f) { tooClose = true; break; }
        }
        if (tooClose) continue;

        TreeInstance tree;

        // Species mix: mostly leafy, a scattering of tall, some small.
        float roll = rng.nextFloat();
        if      (roll < 0.52f) tree.kind = TREE_LEAFY;
        else if (roll < 0.78f) tree.kind = TREE_TALL;
        else                   tree.kind = TREE_SMALL;

        tree.position    = Vec3(x, ground, z);
        tree.heightScale = rng.range(0.78f, 1.35f);
        tree.widthScale  = tree.heightScale * rng.range(0.85f, 1.18f);
        tree.rotation    = rng.range(0.0f, 360.0f);
        tree.swayPhase   = rng.range(0.0f, util::TWO_PI);
        tree.swaySpeed   = rng.range(0.55f, 1.05f);

        // Tall thin trees whip more than squat ones.
        float base = (tree.kind == TREE_TALL) ? 8.5f
                   : (tree.kind == TREE_SMALL ? 4.5f : 6.0f);
        tree.swayAmount = base * rng.range(0.8f, 1.25f);

        float trunkShade = rng.range(0.0f, 1.0f);
        tree.trunkColor = util::lerpv(Vec3(0.30f, 0.21f, 0.13f),
                                      Vec3(0.44f, 0.32f, 0.20f), trunkShade);

        float leafShade = rng.nextFloat();
        Vec3 leafA(0.13f, 0.32f, 0.14f);
        Vec3 leafB(0.28f, 0.48f, 0.20f);
        if (tree.kind == TREE_TALL) { leafA = Vec3(0.11f, 0.28f, 0.17f); leafB = Vec3(0.20f, 0.42f, 0.24f); }
        if (tree.kind == TREE_SMALL){ leafA = Vec3(0.20f, 0.40f, 0.16f); leafB = Vec3(0.36f, 0.54f, 0.22f); }
        tree.foliageColor = util::lerpv(leafA, leafB, leafShade);

        float totalHeight = canopyBaseHeight(tree.kind) * tree.heightScale + 3.0f;
        tree.cullRadius = totalHeight * 0.85f + 2.0f;

        m_trees.push_back(tree);
    }

    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "Forest generated: %d trees (%d placement attempts).",
                  static_cast<int>(m_trees.size()), attempts);
    util::logInfo(buffer);
}

void Forest::buildRenderData() {
    if (!m_listsBuilt) buildSpeciesLists();
    util::logInfo("Tree species compiled into display lists.");
}

void Forest::destroyRenderData() {
    if (!m_listsBuilt) return;
    for (int k = 0; k < TREE_KIND_COUNT; ++k) {
        for (int l = 0; l < TREE_LOD_COUNT; ++l) {
            if (m_trunkLists[k][l])   glDeleteLists(m_trunkLists[k][l], 1);
            if (m_foliageLists[k][l]) glDeleteLists(m_foliageLists[k][l], 1);
            m_trunkLists[k][l] = 0;
            m_foliageLists[k][l] = 0;
        }
    }
    m_listsBuilt = false;
}

// ---------------------------------------------------------------------------
//  Rendering
// ---------------------------------------------------------------------------
void Forest::render(const RenderContext& context) const {
    if (!m_listsBuilt) return;

    for (size_t i = 0; i < m_trees.size(); ++i) {
        const TreeInstance& tree = m_trees[i];

        // Cull first: the cheapest tree is the one never submitted.
        Vec3 centre(tree.position.x,
                    tree.position.y + tree.cullRadius * 0.6f,
                    tree.position.z);
        if (!context.frustum.sphereVisible(centre, tree.cullRadius)) continue;

        int lod = context.levelOfDetail(tree.position,
                                        Config::LOD_NEAR_DISTANCE,
                                        Config::LOD_FAR_DISTANCE);
        if (lod > 2) continue;

        ++context.visibleTrees;

        glPushMatrix();
        glTranslatef(tree.position.x, tree.position.y, tree.position.z);
        glRotatef(tree.rotation, 0.0f, 1.0f, 0.0f);
        glScalef(tree.widthScale, tree.heightScale, tree.widthScale);

        // --- trunk: planted, does not move ---
        glColor3f(tree.trunkColor.x, tree.trunkColor.y, tree.trunkColor.z);
        if (m_trunkLists[tree.kind][lod]) glCallList(m_trunkLists[tree.kind][lod]);

        // --- canopy: sways ---
        // The phase offset per tree is what stops the forest moving as one
        // rigid object, and the two different frequencies keep it organic.
        // Two frequencies per axis: a slow heave plus a faster flutter. A
        // single sine makes a whole forest rock like a metronome.
        float phase = context.windPhase * tree.swaySpeed + tree.swayPhase;
        float swayX = (std::sin(phase) * 1.0f
                     + std::sin(phase * 2.7f + 0.8f) * 0.28f)
                    * tree.swayAmount * context.wind;
        float swayZ = (std::sin(phase * 0.73f + 1.9f) * 1.0f
                     + std::sin(phase * 2.1f + 2.6f) * 0.24f)
                    * tree.swayAmount * 0.60f * context.wind;

        float pivot = canopyBaseHeight(tree.kind);

        glPushMatrix();
        glTranslatef(0.0f, pivot, 0.0f);
        glRotatef(swayX, 0.0f, 0.0f, 1.0f);
        glRotatef(swayZ, 1.0f, 0.0f, 0.0f);

        glColor3f(tree.foliageColor.x, tree.foliageColor.y, tree.foliageColor.z);
        if (m_foliageLists[tree.kind][lod]) glCallList(m_foliageLists[tree.kind][lod]);
        glPopMatrix();

        glPopMatrix();
    }
}
