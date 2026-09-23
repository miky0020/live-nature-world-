// ============================================================================
//  CharacterManager.h - who is where, and what they are doing.
//
//  Three groups, matching sections 15-17 of the brief:
//
//    * the campfire circle - sitting, with exactly one person talking at a
//      time and the rest listening and turning toward whoever has the floor
//    * the gathering area  - dancing and emoting, on staggered timers
//    * walkers             - following the ring path around the village
//
//  The conversation logic is the part that sells it: a single "speaker" token
//  passed around the circle on an irregular timer, with everyone else aimed at
//  the speaker. That reads as a conversation in a way that random gesturing
//  never does.
// ============================================================================
#ifndef LIVINGISLAND_CHARACTERMANAGER_H
#define LIVINGISLAND_CHARACTERMANAGER_H

#include "Character.h"
#include "RenderContext.h"
#include "Utilities.h"

#include <vector>

class Terrain;

class CharacterManager {
public:
    CharacterManager();

    void initialize(const Terrain& terrain, unsigned seed,
                    const util::Vec3& campfire,
                    const util::Vec3& danceArea);

    void update(float deltaTime, const Terrain& terrain);
    void render(const RenderContext& context) const;

    // Cycles the emote group through wave / cheer / dance on demand, so the
    // presenter can trigger it during a demo.
    void triggerEmote();

    int count() const { return static_cast<int>(m_characters.size()); }

private:
    void updateConversation(float deltaTime);
    void updateEmotes(float deltaTime);

    std::vector<Character> m_characters;

    // indices into m_characters
    std::vector<int> m_sitters;
    std::vector<int> m_dancers;
    std::vector<int> m_walkers;

    util::Vec3 m_campfire;
    util::Vec3 m_danceArea;

    int   m_speaker;          // index into m_sitters, -1 for a lull
    float m_speakerTimer;
    float m_speakerDuration;

    float m_emoteTimer;
    float m_emoteInterval;
    int   m_emoteCycle;

    util::Rng m_rng;
};

#endif // LIVINGISLAND_CHARACTERMANAGER_H
