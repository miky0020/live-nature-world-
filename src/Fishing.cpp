// ============================================================================
// src/Fishing.cpp
// ============================================================================
#include "Fishing.h"
#include "Config.h"

#include <cstdio>

namespace {
    const float BITE_REACTION_SECONDS = 1.4f;   // window to press E once it bites
}

FishingSystem::FishingSystem()
    : m_state(FISHING_IDLE)
    , m_timer(0.0f)
    , m_rng(Config::WORLD_SEED ^ 0x71570Du)
    , m_caughtCount(0)
    , m_lastCatchSuccess(false)
{
}

void FishingSystem::update(float deltaTime) {
    if (m_state == FISHING_IDLE) return;

    m_timer -= deltaTime;
    if (m_timer > 0.0f) return;

    switch (m_state) {
        case FISHING_WAITING:
            m_state = FISHING_BITING;
            m_timer = BITE_REACTION_SECONDS;
            break;

        case FISHING_BITING:
            // Missed the window - the fish gets away.
            m_lastCatchSuccess = false;
            m_state = FISHING_COOLDOWN;
            m_timer = Config::FISH_COOLDOWN_SECONDS;
            break;

        case FISHING_REELING:
            ++m_caughtCount;
            m_lastCatchSuccess = true;
            m_state = FISHING_COOLDOWN;
            m_timer = Config::FISH_COOLDOWN_SECONDS;
            break;

        case FISHING_COOLDOWN:
            m_state = FISHING_IDLE;
            break;

        default:
            break;
    }
}

void FishingSystem::onInteract() {
    if (m_state == FISHING_IDLE) {
        m_state = FISHING_WAITING;
        m_timer = m_rng.range(Config::FISH_BITE_MIN_SECONDS, Config::FISH_BITE_MAX_SECONDS);
    } else if (m_state == FISHING_BITING) {
        m_state = FISHING_REELING;
        m_timer = Config::FISH_REEL_SECONDS;
    }
    // WAITING / REELING / COOLDOWN: already committed to the current step -
    // an extra press does nothing.
}

void FishingSystem::cancel() {
    m_state = FISHING_IDLE;
    m_timer = 0.0f;
}

std::string FishingSystem::statusText() const {
    char buffer[96];
    switch (m_state) {
        case FISHING_WAITING:
            return "Fishing... wait for a bite";
        case FISHING_BITING:
            return "Fish on the line! Press E!";
        case FISHING_REELING:
            return "Reeling it in...";
        case FISHING_COOLDOWN:
            if (m_lastCatchSuccess) {
                std::snprintf(buffer, sizeof(buffer), "Caught a fish! (%d so far)", m_caughtCount);
            } else {
                std::snprintf(buffer, sizeof(buffer), "The fish got away...");
            }
            return std::string(buffer);
        default:
            return std::string();
    }
}
