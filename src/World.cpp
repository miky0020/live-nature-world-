// ============================================================================
// src/World.cpp
// ============================================================================
#include "World.h"
#include "Camera.h"
#include "Config.h"

using util::Vec3;

World::World()
    : m_swingOccupied(false)
    , m_swingPump(0.0f)
    , m_elapsed(0.0f)
    , m_timeOfDay(Config::DEFAULT_TIME_OF_DAY)
    , m_timeSpeed(1.0f)
    , m_dayLengthSeconds(Config::DEFAULT_DAY_LENGTH)
{
}

// Keep-out areas, consulted by every scatterer. Section 27 asks for exactly
// this: randomisation that cannot produce impossible placements.
void World::buildExclusionZones() {
    m_exclusions.clear();

    float campfireY = m_terrain.heightAt(Config::CAMPFIRE_X, Config::CAMPFIRE_Z);
    m_exclusions.push_back(util::Zone(
        Vec3(Config::CAMPFIRE_X, campfireY, Config::CAMPFIRE_Z), 6.5f));

    float sakuraY = m_sakura.position().y;
    m_exclusions.push_back(util::Zone(
        Vec3(Config::SAKURA_X, sakuraY, Config::SAKURA_Z), 2.2f));

    float treehouseY = m_terrain.heightAt(Config::TREEHOUSE_X, Config::TREEHOUSE_Z);
    m_exclusions.push_back(util::Zone(
        Vec3(Config::TREEHOUSE_X, treehouseY, Config::TREEHOUSE_Z), 8.0f));

    float tentY = m_tent.position().y;
    m_exclusions.push_back(util::Zone(
        Vec3(Config::TENT_X, tentY, Config::TENT_Z), Config::TENT_EXCLUSION_RADIUS));

    float danceY = m_terrain.heightAt(Config::DANCE_X, Config::DANCE_Z);
    m_exclusions.push_back(util::Zone(
        Vec3(Config::DANCE_X, danceY, Config::DANCE_Z), 5.5f));

    Vec3 swingAnchor = m_sakura.swingAnchor();
    m_exclusions.push_back(util::Zone(
        Vec3(swingAnchor.x, m_terrain.heightAt(swingAnchor.x, swingAnchor.z), swingAnchor.z), 2.0f));

    // Lantern posts sit just outside the ring path.
    for (int i = 0; i < 6; ++i) {
        float angle = (static_cast<float>(i) / 6.0f) * util::TWO_PI;
        float x = std::cos(angle) * (Config::PATH_RING_RADIUS + 1.6f);
        float z = std::sin(angle) * (Config::PATH_RING_RADIUS + 1.6f);
        m_exclusions.push_back(util::Zone(Vec3(x, m_terrain.heightAt(x, z), z), 1.8f));
    }
}

// What the player actually collides with (section 16): a small circle per
// tree trunk - not the much larger exclusion-zone radii above, which exist
// to keep vegetation from spawning on top of landmarks, not to block a
// person from walking near one. Built once, after both the forest and the
// treehouse exist, since neither moves afterward.
void World::buildCollisionObstacles() {
    m_collisionObstacles.clear();

    const std::vector<TreeInstance>& trees = m_forest.trees();
    m_collisionObstacles.reserve(trees.size() + 2);
    for (size_t i = 0; i < trees.size(); ++i) {
        // A small trunk-sized circle, not the tree's much larger cullRadius
        // (that's a visual bounding sphere for frustum culling, not
        // something a person should bump into three units from the trunk).
        float trunkRadius = 0.35f * trees[i].widthScale;
        m_collisionObstacles.push_back(
            util::Zone(trees[i].position, trunkRadius));
    }

    // The treehouse's central trunk and support struts, not its whole
    // exclusion zone - the player should be able to walk right up to it.
    m_collisionObstacles.push_back(
        util::Zone(m_environment.treehousePosition(), 1.3f));

    // Hero Sakura trunk collision. The canopy remains walk-through.
    m_collisionObstacles.push_back(
        util::Zone(m_sakura.position(), 0.72f));
}

// Registers every interaction prompt once landmarks exist. One reusable
// resolver (Interaction.h) rather than a per-landmark proximity check
// scattered through Application/World.
void World::buildInteractionZones() {
    m_interaction.addZone(m_campfire.position(), Config::CAMPFIRE_INTERACT_RADIUS,
                          "Warm and cozy by the fire");
    m_interaction.addZone(m_environment.treehousePosition(), Config::TREEHOUSE_INTERACT_RADIUS,
                          "Press E to climb the treehouse");
    m_interaction.addZone(m_swing.position(), Config::SWING_INTERACT_RADIUS,
                          "Press E to use Swing");
    m_interaction.setLakeCheck(Config::LAKE_INTERACT_RADIUS, Config::WATER_LEVEL,
                               "Press E to fish");
}

void World::initialize(unsigned seed) {
    util::logInfo("Initializing world...");

    // --- terrain first: everything else is placed relative to it -----------
    m_terrain.generate(seed);
    m_terrain.buildRenderData();

    // The hero Sakura is placed before vegetation generation so the shared
    // exclusion system can keep grass/flowers/rocks from growing through it.
    m_sakura.initialize(m_terrain, Config::SAKURA_X, Config::SAKURA_Z, seed);
    m_tent.initialize(m_terrain, Config::TENT_X, Config::TENT_Z);

    buildExclusionZones();

    // --- environment -------------------------------------------------------
    m_water.initialize(Config::CAM_FAR_PLANE * 1.4f, 68);
    m_sky.initialize(16, 32);
    m_sky.buildRenderData();
    m_lighting.initialize();

    // --- vegetation and structures ----------------------------------------
    m_forest.generate(m_terrain, seed, m_exclusions);
    m_forest.buildRenderData();

    m_vegetation.generate(m_terrain, seed, m_exclusions);
    m_vegetation.buildRenderData();

    m_environment.initialize(m_terrain, seed);
    m_environment.buildRenderData();

    m_sakura.buildRenderData();

    buildCollisionObstacles();

    m_campfire.initialize(m_terrain, Config::CAMPFIRE_X, Config::CAMPFIRE_Z, seed);
    m_campfire.buildRenderData();

    Vec3 swingAnchor = m_sakura.swingAnchor();
    m_swing.initializeTreeMounted(m_terrain, swingAnchor.x, swingAnchor.z,
                                  swingAnchor.y - m_terrain.heightAt(swingAnchor.x, swingAnchor.z),
                                  m_sakura.swingFacing(), seed);
    m_swing.buildRenderData();
    m_tent.buildRenderData();
    m_sakuraPetals.initialize(m_sakura.position(), seed, Config::MAX_SAKURA_PETALS);

    buildInteractionZones();

    // --- people ------------------------------------------------------------
    float danceY = m_terrain.heightAt(Config::DANCE_X, Config::DANCE_Z);
    m_characters.initialize(m_terrain, seed,
                            m_campfire.position(),
                            Vec3(Config::DANCE_X, danceY, Config::DANCE_Z));

    // --- weather and particles ---------------------------------------------
    m_rain.initialize(Config::MAX_RAIN_PARTICLES);
    m_fireflies.initialize(m_terrain, seed, Config::FIREFLY_COUNT);
    m_weather.setType(WEATHER_CLEAR);

    // --- audio (optional; never fatal) -------------------------------------
    m_audio.initialize("assets/audio/");

    m_lighting.update(m_timeOfDay, m_weather.cloud());

    util::logInfo("World initialized successfully.");
}

void World::shutdown() {
    m_audio.shutdown();
    m_swing.destroyRenderData();
    m_tent.destroyRenderData();
    m_sakura.destroyRenderData();
    m_campfire.destroyRenderData();
    m_environment.destroyRenderData();
    m_vegetation.destroyRenderData();
    m_forest.destroyRenderData();
    m_sky.destroyRenderData();
    m_terrain.destroyRenderData();
    util::logInfo("World resources released.");
}

// ---------------------------------------------------------------------------
//  Control values, mirrored with the UI sliders
// ---------------------------------------------------------------------------
ControlValues World::controls() const {
    ControlValues values;
    values.timeOfDay     = m_timeOfDay;
    values.timeSpeed     = m_timeSpeed;
    values.rainIntensity = m_weather.rain();
    values.windStrength  = m_weather.wind();
    values.cloudDensity  = m_weather.cloud();
    return values;
}

void World::applyControls(const ControlValues& values) {
    m_timeOfDay = util::wrapf(values.timeOfDay, 24.0f);
    m_timeSpeed = util::clampf(values.timeSpeed, 0.0f, 10.0f);
    m_weather.overrideRain(values.rainIntensity);
    m_weather.overrideWind(values.windStrength);
    m_weather.overrideCloud(values.cloudDensity);
}

void World::setTimeOfDay(float hours) { m_timeOfDay = util::wrapf(hours, 24.0f); }
void World::setTimeSpeed(float speed) { m_timeSpeed = util::clampf(speed, 0.0f, 10.0f); }

// ---------------------------------------------------------------------------
//  Update
// ---------------------------------------------------------------------------
void World::update(float deltaTime, const Camera& camera) {
    m_elapsed += deltaTime;

    // --- clock -------------------------------------------------------------
    if (m_timeSpeed > 0.0f && m_dayLengthSeconds > 0.0f) {
        float hoursPerSecond = 24.0f / m_dayLengthSeconds;
        m_timeOfDay = util::wrapf(m_timeOfDay + hoursPerSecond * m_timeSpeed * deltaTime,
                                  24.0f);
    }

    m_weather.update(deltaTime);
    m_lighting.update(m_timeOfDay, m_weather.cloud());

    // --- build this frame's shared context ---------------------------------
    // Everything downstream reads from here, so the whole world agrees on the
    // wind, the time and the light.
    m_context.cameraPosition = camera.position();
    m_context.time           = m_elapsed;
    m_context.windPhase      = m_weather.windPhase();
    m_context.wind           = m_weather.gustWind();
    m_context.daylight       = m_lighting.daylight();
    m_context.nightFactor    = 1.0f - m_lighting.daylight();
    m_context.rainIntensity  = m_weather.rain();
    m_context.cloudDensity   = m_weather.cloud();
    m_context.sunColor       = m_lighting.sunColor();
    m_context.ambientColor   = m_lighting.ambientColor();
    m_context.fogColor       = m_lighting.fogColor();
    m_context.drawDistance   = Config::DRAW_DISTANCE;
    m_context.liveParticles  = 0;

    // --- simulation --------------------------------------------------------
    m_sky.update(deltaTime, m_context);
    m_water.update(deltaTime, m_context.wind);
    m_environment.update(deltaTime, m_context);
    m_sakura.update(deltaTime, m_context);
    m_campfire.update(deltaTime, m_context);
    m_swing.update(deltaTime, m_swingOccupied, m_swingPump);
    m_characters.update(deltaTime, m_terrain);
    m_rain.update(deltaTime, m_context, m_terrain);
    m_fireflies.update(deltaTime, m_context);
    m_sakuraPetals.update(deltaTime, m_context, m_terrain);

    // --- audio mix ---------------------------------------------------------
    AudioEnvironment audioEnvironment;
    audioEnvironment.daylight         = m_context.daylight;
    audioEnvironment.rainIntensity    = m_context.rainIntensity;
    audioEnvironment.windStrength     = m_context.wind;
    audioEnvironment.campfireDistance = util::distanceXZ(camera.position(),
                                                         m_campfire.position());
    audioEnvironment.campfireGlow     = m_campfire.glow();
    m_audio.update(deltaTime, audioEnvironment);
}

// ---------------------------------------------------------------------------
//  Fog
// ---------------------------------------------------------------------------
void World::applyFog() {
    Vec3 fog = m_lighting.fogColor();
    const GLfloat color[] = { fog.x, fog.y, fog.z, 1.0f };

    // Thickens with cloud cover and rain, and a little at night. Kept gentle
    // on purpose: atmosphere, not a grey wall (section 23).
    float density = 0.0021f
                  + m_context.cloudDensity  * 0.0013f
                  + m_context.rainIntensity * 0.0026f
                  + m_context.nightFactor   * 0.0011f;

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogfv(GL_FOG_COLOR, color);
    glFogf(GL_FOG_DENSITY, density);
    glHint(GL_FOG_HINT, GL_NICEST);
}

// ---------------------------------------------------------------------------
//  Render - the order here is load-bearing
// ---------------------------------------------------------------------------
void World::render(const Camera& camera) {
    Vec3 fog = m_lighting.fogColor();
    glClearColor(fog.x, fog.y, fog.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    camera.applyView();

    // The frustum has to be extracted after the view matrix is set, or every
    // cull test this frame would be against the previous frame's camera.
    m_context.frustum.extractFromCurrentMatrices();

    m_context.visibleTrees = 0;
    m_context.visibleGrass = 0;
    m_context.visibleProps = 0;
    m_context.visibleCharacters = 0;

    // --- 1. sky backdrop: no depth, no fog, no lighting --------------------
    m_sky.renderBackdrop(m_context,
                         m_lighting.horizonColor(),
                         m_lighting.zenithColor(),
                         m_lighting.sunDirection());

    // --- 2. lights ---------------------------------------------------------
    // Three lights, and no more. Fixed-function lighting is per-vertex and
    // every extra light costs on every vertex in the scene (section 22).
    m_lighting.apply();
    m_campfire.applyLight(GL_LIGHT1, m_context.nightFactor);
    m_environment.applyLanternLight(GL_LIGHT2, m_context.nightFactor);

    applyFog();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Opaque geometry never blends, no matter what the last thing drawn left
    // behind. This is the fix for a real bug: Sky's sun/moon glow sets an
    // ADDITIVE blend function (glBlendFunc(GL_SRC_ALPHA, GL_ONE)) and only
    // wraps it in glPushAttrib(GL_ENABLE_BIT) - which saves whether blending
    // is on/off, but NOT which blend function is active (that is
    // GL_COLOR_BUFFER_BIT). So the additive function survived popAttrib and
    // leaked straight into the terrain draw that follows it, adding every
    // opaque surface's colour on top of the sky instead of replacing it -
    // which is exactly what "everything is white" looks like. Disabling
    // blending outright for the whole opaque pass makes this class of bug
    // impossible regardless of what any earlier draw call left set, and it is
    // also free performance: nothing here needs the blend stage at all.
    glDisable(GL_BLEND);

    // --- 3. opaque geometry ------------------------------------------------
    m_terrain.render();
    m_vegetation.renderRocks(m_context);
    m_environment.renderSolid(m_context);
    m_campfire.renderSolid(m_context);
    m_sakura.render(m_context);
    m_swing.renderSolid(m_context);
    m_tent.render(m_context);
    m_forest.render(m_context);
    m_characters.render(m_context);

    // Grass and flowers are two-sided, so they manage their own cull state.
    m_vegetation.renderGrassAndFlowers(m_context);

    // --- 4. emissive surfaces (windows, lanterns) --------------------------
    m_environment.renderEmissive(m_context);

    // --- 5. transparent passes, after all opaque depth exists --------------
    // Re-establish a known blend state explicitly rather than trusting
    // whatever renderEmissive() left behind - same reasoning as above.
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_water.render(camera.position(), m_lighting.waterColor(),
                   m_lighting.sunDirection(), m_lighting.sunColor(),
                   m_lighting.daylight(), 0.74f, m_terrain);
    m_sky.renderClouds(m_context);
    m_campfire.renderEffects(m_context);
    m_fireflies.render(m_context);
    m_sakuraPetals.render(m_context);
    m_rain.render(m_context);

    glDisable(GL_FOG);
    glDisable(GL_LIGHT1);
    glDisable(GL_LIGHT2);
}