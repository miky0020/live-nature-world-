// ============================================================================
//  Primitives.h - the small vocabulary of shapes the whole world is built from.
//
//  Section 48 of the brief asks for procedural fallback geometry so the project
//  never depends on external models. Rather than scattering glBegin loops
//  through every system, everything is assembled from these few primitives:
//  trees, characters, the treehouse, rocks and the campfire all reduce to
//  cylinders, spheres, blobs and boxes.
//
//  Conventions:
//    * cylinders and cones grow along +Y with their base at y = 0
//    * spheres and blobs are centred on the origin
//    * boxes are centred on the origin
//    * no colour is ever set here, so callers stay in control and these can be
//      safely compiled into display lists that are tinted per instance
// ============================================================================
#ifndef LIVINGISLAND_PRIMITIVES_H
#define LIVINGISLAND_PRIMITIVES_H

#include "GLIncludes.h"
#include "Utilities.h"

namespace prim {

// Tapered cylinder. topRadius of 0 gives a cone.
void drawCylinder(float baseRadius, float topRadius, float height,
                  int slices, bool capBase = true, bool capTop = true);

void drawSphere(float radius, int slices, int stacks);

// Sphere with per-vertex radius jitter - the workhorse for rocks and for
// foliage masses that should not look like billiard balls. The jitter is
// hashed from the vertex indices, so a given seed always yields the same
// shape and the seam at the back closes properly.
void drawBlob(float radius, int slices, int stacks, unsigned seed,
              float jitter, float flatten = 1.0f);

void drawBox(float sizeX, float sizeY, float sizeZ);

// Four-sided pyramid, base at y = 0, centred on XZ. Used for roofs.
void drawPyramid(float baseX, float baseZ, float height);

// Flat quad in the XZ plane at y = 0, facing up.
void drawQuadXZ(float sizeX, float sizeZ);

// Upright quad in the XY plane, base at y = 0, centred on X. Used for grass
// blades, leaves and flower petals.
void drawUprightQuad(float width, float height);

// Two upright quads crossed at 90 degrees - a cheap stand-in for a volumetric
// tuft that reads correctly from any viewing angle.
void drawCrossQuads(float width, float height);

// A single curved, tapering grass blade built from several stacked segments.
// The curve is what makes grass read as grass rather than as cardboard: a
// straight quad always looks like a signpost. `bend` is how far the tip leans
// (in local +Z), `segments` controls the smoothness of the arc.
void drawGrassBlade(float width, float height, float bend, int segments);

// A leaf cluster: a small fan of tapered blades used to build tree canopies
// with real silhouette detail instead of smooth spheres.
void drawLeafSpray(float radius, float leafLength, int leafCount, unsigned seed);

} // namespace prim

#endif // LIVINGISLAND_PRIMITIVES_H
