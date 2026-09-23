// ============================================================================
//  Interaction.h - one reusable "what's the player near right now" resolver.
//
//  Registered once at load time (campfire, treehouse, swing, ...); resolve()
//  is called every frame and returns at most one active prompt - the nearest
//  thing the player is standing close enough to. Not a per-landmark system:
//  every proximity check in the game goes through this one class.
// ============================================================================
#ifndef LIVINGISLAND_INTERACTION_H
#define LIVINGISLAND_INTERACTION_H

#include "Utilities.h"

#include <string>
#include <vector>

class Terrain;

class InteractionSystem {
public:
    InteractionSystem();

    // A fixed-point trigger - campfire, treehouse, swing: anything with one
    // place. The label itself carries what it means; callers match on it
    // (Application checks label.find("treehouse")/"swing"/"fish").
    void addZone(const util::Vec3& position, float radius, const std::string& label);

    // The lake has no single point - "near it" means "near the shoreline" -
    // so it is checked against the terrain each call instead of one fixed
    // position. radius here is how far above the waterline still counts.
    void setLakeCheck(float radius, float waterLevel, const std::string& label);

    // Returns true and fills outLabel with the nearest active prompt, or
    // false if the player isn't near anything interactive right now.
    bool resolve(const util::Vec3& playerPosition, const Terrain& terrain,
                 std::string& outLabel) const;

private:
    struct Zone {
        util::Vec3  position;
        float       radius;
        std::string label;
    };

    std::vector<Zone> m_zones;

    bool        m_lakeEnabled;
    float       m_lakeRadius;
    float       m_waterLevel;
    std::string m_lakeLabel;
};

#endif // LIVINGISLAND_INTERACTION_H
