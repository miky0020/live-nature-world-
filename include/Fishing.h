// ============================================================================
//  Fishing.h - the lake minigame: cast, wait, react in time, reel in.
//
//  Driven entirely by Application (onInteract() on each 'E' press near the
//  lake, update() every frame). It owns no rendering - it only ever changes
//  what the HUD prompt says, reusing the interaction prompt display rather
//  than adding a second on-screen text system.
// ============================================================================
#ifndef LIVINGISLAND_FISHING_H
#define LIVINGISLAND_FISHING_H

#include "Utilities.h"

#include <string>

enum FishingState {
    FISHING_IDLE = 0,
    FISHING_WAITING,
    FISHING_BITING,
    FISHING_REELING,
    FISHING_COOLDOWN
};

class FishingSystem {
public:
    FishingSystem();

    void update(float deltaTime);
    void onInteract();   // 'E' pressed while near the lake
    void cancel();       // player walked away mid-cycle

    bool isActive() const { return m_state != FISHING_IDLE; }
    std::string statusText() const;

private:
    FishingState m_state;
    float        m_timer;
    util::Rng    m_rng;
    int          m_caughtCount;
    bool         m_lastCatchSuccess;
};

#endif // LIVINGISLAND_FISHING_H
