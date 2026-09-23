// ============================================================================
//  RenderContext.h - everything a renderer needs to know about *this* frame.
//
//  Passing one struct keeps the render signatures stable: when a later system
//  needs the wind value or the night factor, it reads it from here instead of
//  threading a new parameter through half the project.
// ============================================================================
#ifndef LIVINGISLAND_RENDERCONTEXT_H
#define LIVINGISLAND_RENDERCONTEXT_H

#include "Frustum.h"
#include "Utilities.h"

struct RenderContext {
    util::Vec3 cameraPosition;
    Frustum    frustum;

    float time;          // seconds since startup - drives every animation
    float windPhase;     // integrated wind time, so gusts never jump
    float wind;          // 0..1 effective wind strength right now
    float daylight;      // 0 at night, 1 at noon
    float nightFactor;   // 1 - daylight, cached because it is used constantly

    float rainIntensity; // 0..1
    float cloudDensity;  // 0..1

    util::Vec3 sunColor;
    util::Vec3 ambientColor;
    util::Vec3 fogColor;

    float drawDistance;

    // Populated by the renderers so the debug panel can report real numbers.
    mutable int visibleTrees;
    mutable int visibleGrass;
    mutable int visibleProps;
    mutable int visibleCharacters;
    mutable int liveParticles;

    RenderContext()
        : time(0.0f), windPhase(0.0f), wind(0.0f)
        , daylight(1.0f), nightFactor(0.0f)
        , rainIntensity(0.0f), cloudDensity(0.0f)
        , drawDistance(320.0f)
        , visibleTrees(0), visibleGrass(0), visibleProps(0)
        , visibleCharacters(0), liveParticles(0) {}

    // Shared helper: how detailed should something at this distance be?
    // 0 = full, 1 = simplified, 2 = crude, 3 = do not draw.
    int levelOfDetail(const util::Vec3& position, float nearLod, float farLod) const {
        float distance = util::distanceXZ(position, cameraPosition);
        if (distance > drawDistance) return 3;
        if (distance < nearLod) return 0;
        if (distance < farLod)  return 1;
        return 2;
    }
};

#endif // LIVINGISLAND_RENDERCONTEXT_H
