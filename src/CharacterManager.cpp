#include "CharacterManager.h"
#include "Config.h"
#include "Terrain.h"

#include <cstdio>

using util::Vec3;

CharacterManager::CharacterManager()
    : m_speaker(-1)
    , m_speakerTimer(0.0f)
    , m_speakerDuration(4.0f)
    , m_emoteTimer(0.0f)
    , m_emoteInterval(9.0f)
    , m_emoteCycle(0)
    , m_rng(Config::WORLD_SEED ^ 0xC4A5u)
{
}

void CharacterManager::initialize(const Terrain& terrain, unsigned seed,
                                  const Vec3& campfire, const Vec3& danceArea)
{
    m_characters.clear();
    m_sitters.clear();
    m_dancers.clear();
    m_walkers.clear();

    m_campfire  = campfire;
    m_danceArea = danceArea;
    m_rng.reseed(seed ^ 0xC4A5u);

    const int total = Config::CHARACTER_COUNT;

    // Split the population: roughly half around the fire, a third dancing,
    // the rest walking.
    int sitterCount = total / 2;                 // 4 of 9
    int dancerCount = (total - sitterCount) / 2 + 1;  // 3 of 9
    int walkerCount = total - sitterCount - dancerCount;

    // --- the campfire circle ----------------------------------------------
    for (int i = 0; i < sitterCount; ++i) {
        // Seat them on the same radius the benches were placed on, offset so
        // nobody sits inside a bench.
        float angle = (static_cast<float>(i) / static_cast<float>(sitterCount)) * util::TWO_PI
                    + 0.35f + 0.28f;
        float radius = 2.85f;

        float x = campfire.x + std::cos(angle) * radius;
        float z = campfire.z + std::sin(angle) * radius;
        float y = terrain.heightAt(x, z);

        Character character;
        character.initialize(Vec3(x, y, z), 0.0f, ANIM_SITTING,
                             seed + static_cast<unsigned>(i) * 977u);
        character.faceToward(campfire);
        character.setFacing(util::toDegrees(std::atan2(campfire.x - x, campfire.z - z)));

        m_sitters.push_back(static_cast<int>(m_characters.size()));
        m_characters.push_back(character);
    }

    // --- the gathering / emote area ---------------------------------------
    for (int i = 0; i < dancerCount; ++i) {
        float angle = (static_cast<float>(i) / static_cast<float>(dancerCount)) * util::TWO_PI;
        float radius = m_rng.range(1.4f, 2.6f);

        float x = danceArea.x + std::cos(angle) * radius;
        float z = danceArea.z + std::sin(angle) * radius;
        float y = terrain.heightAt(x, z);

        Character character;
        character.initialize(Vec3(x, y, z), m_rng.range(0.0f, 360.0f), ANIM_DANCING,
                             seed + 5000u + static_cast<unsigned>(i) * 613u);
        character.faceToward(danceArea);

        m_dancers.push_back(static_cast<int>(m_characters.size()));
        m_characters.push_back(character);
    }

    // --- walkers on the ring path -----------------------------------------
    for (int i = 0; i < walkerCount; ++i) {
        std::vector<Vec3> waypoints;
        const int stepCount = 10;
        float startAngle = (static_cast<float>(i) / static_cast<float>(walkerCount)) * util::TWO_PI;

        for (int s = 0; s < stepCount; ++s) {
            float angle = startAngle
                        + (static_cast<float>(s) / static_cast<float>(stepCount)) * util::TWO_PI;
            float x = std::cos(angle) * Config::PATH_RING_RADIUS;
            float z = std::sin(angle) * Config::PATH_RING_RADIUS;
            waypoints.push_back(Vec3(x, terrain.heightAt(x, z), z));
        }

        Character character;
        character.initialize(waypoints[0], 0.0f, ANIM_WALKING,
                             seed + 9000u + static_cast<unsigned>(i) * 331u);
        character.setPatrol(waypoints);

        m_walkers.push_back(static_cast<int>(m_characters.size()));
        m_characters.push_back(character);
    }

    // Open the conversation with someone already mid-sentence.
    if (!m_sitters.empty()) {
        m_speaker = 0;
        m_characters[static_cast<size_t>(m_sitters[0])].setState(ANIM_TALKING);
        m_speakerTimer = 0.0f;
        m_speakerDuration = m_rng.range(3.5f, 7.0f);
    }

    char buffer[160];
    std::snprintf(buffer, sizeof(buffer),
                  "Characters created: %d total (%d at the fire, %d in the gathering area, %d walking).",
                  static_cast<int>(m_characters.size()),
                  static_cast<int>(m_sitters.size()),
                  static_cast<int>(m_dancers.size()),
                  static_cast<int>(m_walkers.size()));
    util::logInfo(buffer);
}

// ---------------------------------------------------------------------------
//  Conversation: one speaker at a time, everyone else listening.
// ---------------------------------------------------------------------------
void CharacterManager::updateConversation(float deltaTime) {
    if (m_sitters.empty()) return;

    m_speakerTimer += deltaTime;
    if (m_speakerTimer < m_speakerDuration) return;

    m_speakerTimer = 0.0f;

    // Occasionally nobody speaks - a natural pause. Without these the circle
    // feels like a machine taking turns.
    bool lull = m_rng.chance(0.18f);

    if (lull) {
        m_speaker = -1;
        m_speakerDuration = m_rng.range(1.2f, 2.8f);
    } else {
        int next = m_speaker;
        // Pick someone other than the current speaker.
        for (int attempt = 0; attempt < 8; ++attempt) {
            int candidate = m_rng.rangeInt(0, static_cast<int>(m_sitters.size()) - 1);
            if (candidate != m_speaker) { next = candidate; break; }
        }
        m_speaker = next;
        m_speakerDuration = m_rng.range(3.0f, 7.5f);
    }

    // Apply the roles.
    for (size_t i = 0; i < m_sitters.size(); ++i) {
        Character& character = m_characters[static_cast<size_t>(m_sitters[i])];

        if (static_cast<int>(i) == m_speaker) {
            character.setState(ANIM_TALKING);
            character.clearLookTarget();
            character.faceToward(m_campfire);
        } else {
            character.setState(ANIM_SITTING);
            if (m_speaker >= 0 && m_rng.chance(0.7f)) {
                // Turn toward whoever is speaking...
                character.setLookTarget(
                    m_characters[static_cast<size_t>(m_sitters[static_cast<size_t>(m_speaker)])].position());
            } else {
                // ...or just watch the fire.
                character.clearLookTarget();
                character.faceToward(m_campfire);
            }
        }
    }
}

// ---------------------------------------------------------------------------
//  Emote group: rotates through dance / wave / cheer on a staggered timer.
// ---------------------------------------------------------------------------
void CharacterManager::updateEmotes(float deltaTime) {
    if (m_dancers.empty()) return;

    m_emoteTimer += deltaTime;
    if (m_emoteTimer < m_emoteInterval) return;

    m_emoteTimer = 0.0f;
    m_emoteInterval = m_rng.range(7.0f, 14.0f);
    triggerEmote();
}

void CharacterManager::triggerEmote() {
    if (m_dancers.empty()) return;

    m_emoteCycle = (m_emoteCycle + 1) % 4;

    for (size_t i = 0; i < m_dancers.size(); ++i) {
        Character& character = m_characters[static_cast<size_t>(m_dancers[i])];

        // Not everyone switches at once: some hold their current emote, which
        // keeps the group from looking like a drill squad.
        if (m_rng.chance(0.25f)) continue;

        AnimationState state;
        switch ((m_emoteCycle + static_cast<int>(i)) % 4) {
            case 0:  state = ANIM_DANCING;  break;
            case 1:  state = ANIM_WAVING;   break;
            case 2:  state = ANIM_CHEERING; break;
            default: state = ANIM_DANCING;  break;
        }
        character.setState(state);
    }
}

void CharacterManager::update(float deltaTime, const Terrain& terrain) {
    updateConversation(deltaTime);
    updateEmotes(deltaTime);

    for (size_t i = 0; i < m_characters.size(); ++i) {
        m_characters[i].update(deltaTime, terrain);
    }
}

void CharacterManager::render(const RenderContext& context) const {
    for (size_t i = 0; i < m_characters.size(); ++i) {
        m_characters[i].render(context);
    }
}
