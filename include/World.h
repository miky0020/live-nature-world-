// ============================================================================
// include/World.h
// ============================================================================
// ============================================================================
//  World.h - owns every simulated system and drives update/render order.
//
//  Application talks to World; World talks to the subsystems. Adding a system
//  means one member, one update() line and one render() line - never a change
//  to the GLUT plumbing.
//
//  The render order in World.cpp follows section 39 exactly, and the reason it
//  matters is the depth buffer: everything opaque has to be in the depth
//  buffer before a single transparent fragment is drawn, or rain will show
//  through trees.
// ============================================================================
#ifndef LIVINGISLAND_WORLD_H
#define LIVINGISLAND_WORLD_H

#include "AudioManager.h"
#include "Config.h"
#include "Campfire.h"
#include "CharacterManager.h"
#include "Environment.h"
#include "Interaction.h"
#include "Lighting.h"
#include "ParticleSystem.h"
#include "RenderContext.h"
#include "Sky.h"
#include "Sakura.h"
#include "Swing.h"
#include "Tent.h"
#include "Terrain.h"
#include "Tree.h"
#include "UI.h"
#include "Vegetation.h"
#include "Water.h"
#include "Weather.h"

#include <string>

class Camera;

class World {
public:
    World();

    void initialize(unsigned seed);
    void update(float deltaTime, const Camera& camera);
    void render(const Camera& camera);
    void shutdown();

    const Terrain& terrain() const { return m_terrain; }

    // --- environment state, mirrored to and from the UI sliders -----------
    ControlValues controls() const;
    void applyControls(const ControlValues& values);

    void setTimeOfDay(float hours);
    void setTimeSpeed(float speed);
    float timeOfDay() const { return m_timeOfDay; }
    float timeSpeed() const { return m_timeSpeed; }

    void setWeatherType(WeatherType type) { m_weather.setType(type); }
    const char* weatherName() const { return m_weather.name(); }

    void triggerEmote() { m_characters.triggerEmote(); }

    AudioManager& audio() { return m_audio; }

    // Handy places to stand for the demo shortcuts.
    util::Vec3 campfirePosition()  const { return m_campfire.position(); }
    util::Vec3 treehousePosition() const { return m_environment.treehousePosition(); }
    float treehousePlatformY() const {
        return m_environment.treehousePosition().y + m_environment.platformHeight();
    }

    // Swing: driven by Application while ridden, always simulated so it
    // settles naturally when nobody's on it.
    void setSwingRider(bool occupied, float pumpInput) {
        m_swingOccupied = occupied;
        m_swingPump = pumpInput;
    }
    util::Vec3 sakuraPosition() const { return m_sakura.position(); }
    util::Vec3 swingPosition()     const { return m_swing.position(); }
    util::Vec3 swingSeatPosition() const { return m_swing.seatPosition(); }
    float swingBaseFacing()        const { return m_swing.baseFacing(); }

    // Swing interaction follows the moving seat, not the fixed ground point
    // below the tree branch. This keeps the prompt/E interaction attached to
    // the actual usable seat.
    bool nearSwing(const util::Vec3& playerPosition) const {
        return util::distanceXZ(playerPosition, m_swing.seatPosition())
             <= Config::SAKURA_SWING_INTERACT_RADIUS;
    }

    // Debug counters, filled in during render.
    const RenderContext& lastContext() const { return m_context; }
    int treeCount()      const { return m_forest.count(); }
    int characterCount() const { return m_characters.count(); }
    int particleCapacity() const {
        return m_rain.capacity() + Config::MAX_SPARK_PARTICLES + Config::MAX_SMOKE_PARTICLES + m_fireflies.capacity() + m_sakuraPetals.capacity();
    }

    // What the player physically bumps into: one circle per tree trunk plus
    // the treehouse's support footprint. Built once at load time, since
    // trees and the treehouse never move - recomputing this every frame
    // would be pure waste. Reuses Zone (already exclusion zones' type)
    // rather than a second "circle" type (section 22).
    const std::vector<util::Zone>& collisionObstacles() const { return m_collisionObstacles; }

    // The nearest active interaction prompt for the player's current
    // position, or false if they aren't near anything (section: "one
    // reusable interaction system, only show the relevant prompt").
    bool interactionPrompt(const util::Vec3& playerPosition, std::string& outLabel) const {
        return m_interaction.resolve(playerPosition, m_terrain, outLabel);
    }

private:
    void buildExclusionZones();
    void buildCollisionObstacles();
    void buildInteractionZones();
    void applyFog();

    Terrain          m_terrain;
    Water            m_water;
    Sky              m_sky;
    Lighting         m_lighting;
    Weather          m_weather;
    Forest           m_forest;
    Vegetation       m_vegetation;
    Environment      m_environment;
    Campfire         m_campfire;
    CharacterManager m_characters;
    RainSystem       m_rain;
    FireflySystem     m_fireflies;
    AudioManager     m_audio;
    InteractionSystem m_interaction;
    SakuraTree       m_sakura;
    SakuraPetalSystem m_sakuraPetals;
    Swing            m_swing;
    Tent             m_tent;
    bool             m_swingOccupied;
    float            m_swingPump;

    RenderContext m_context;
    std::vector<util::Zone> m_exclusions;
    std::vector<util::Zone> m_collisionObstacles;

    float m_elapsed;
    float m_timeOfDay;
    float m_timeSpeed;
    float m_dayLengthSeconds;
};

#endif // LIVINGISLAND_WORLD_H