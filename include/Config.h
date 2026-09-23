// ============================================================================
//  Config.h - every tunable number in the project lives here.
// ============================================================================

#ifndef LIVINGISLAND_CONFIG_H
#define LIVINGISLAND_CONFIG_H

namespace Config {

// ---------------------------------------------------------------- window ---

const int   WINDOW_WIDTH       = 1920;
const int   WINDOW_HEIGHT      = 1080;
const char* const WINDOW_TITLE = "LIVE NATURE";

// ----------------------------------------------------------------- world ---

const unsigned WORLD_SEED     = 20260915u;
const float WORLD_SIZE        = 220.0f;
const int   TERRAIN_RES       = 129;
const float WATER_LEVEL       = -2.5f;
const float VILLAGE_HEIGHT    = 2.0f;
const float VILLAGE_RADIUS    = 14.0f;

// ---------------------------------------------------------------- camera ---

const float CAM_START_X       = 34.0f;
const float CAM_START_Y       = 12.0f;
const float CAM_START_Z       = 40.0f;
const float CAM_SENSITIVITY   = 0.12f;
const float CAM_FOV           = 62.0f;
const float CAM_NEAR_PLANE    = 0.15f;
const float CAM_FAR_PLANE     = 600.0f;

// ------------------------------------------------------------ simulation ---

const float MAX_DELTA_TIME       = 0.10f;
const float DEFAULT_TIME_OF_DAY  = 10.5f;
const float DEFAULT_DAY_LENGTH   = 180.0f;

// ------------------------------------------- population (later stages) -----

const int TREE_COUNT            = 90;
const int GRASS_CLUSTER_COUNT   = 5000;
const int ROCK_COUNT            = 55;
const int FLOWER_COUNT          = 520;
const int CHARACTER_COUNT       = 9;
const int CLOUD_COUNT           = 18;
const int STAR_COUNT            = 420;
const int MAX_RAIN_PARTICLES    = 1800;
const int MAX_SPARK_PARTICLES   = 190;
const int FIREFLY_COUNT          = 96;
const int MAX_SAKURA_PETALS      = 72;

// ---------------------------------------------------------- shoreline ------

const int SHORE_ROCK_COUNT  = 60;
const int SHORE_GRASS_COUNT = 900;

// ------------------------------------------------- landmark placement ------

const float CAMPFIRE_X        =  0.0f;
const float CAMPFIRE_Z        =  0.0f;

// Small A-frame camping tent placed beside the campfire, behind the main
// seating area so it does not crowd the Sakura or swing.
const float TENT_X            =  5.8f;
const float TENT_Z            =  3.9f;
const float TENT_EXCLUSION_RADIUS = 3.0f;

const float TREEHOUSE_X       = -26.0f;
const float TREEHOUSE_Z       =  22.0f;

const float DANCE_X           =  16.0f;
const float DANCE_Z           = -12.0f;

const float PATH_RING_RADIUS  =  20.0f;
const float PATH_HALF_WIDTH   =  1.9f;

// -------------------------------------------------------------- culling ----

const float DRAW_DISTANCE       = 280.0f;
const float LOD_NEAR_DISTANCE   = 32.0f;
const float LOD_FAR_DISTANCE    = 95.0f;

// ---------------------------------------------------------------- player ---

const float PLAYER_WALK_SPEED          = 4.6f;
const float PLAYER_RUN_MULTIPLIER      = 2.0f;
const float PLAYER_ACCEL_RATE          = 11.0f;
const float PLAYER_TURN_RATE           = 10.0f;
const float PLAYER_JUMP_SPEED          = 6.0f;
const float PLAYER_GRAVITY             = 17.0f;
const float PLAYER_EYE_HEIGHT          = 1.65f;
const float PLAYER_COLLIDE_RADIUS      = 0.42f;
const float PLAYER_ANIM_BLEND_SECONDS  = 0.25f;

// ------------------------------------------------------- third-person cam --

const float THIRD_PERSON_DISTANCE  = 6.5f;
const float THIRD_PERSON_HEIGHT    = 2.6f;
const float THIRD_PERSON_DAMP_RATE = 9.0f;

// ------------------------------------------------------------ interaction --

const float LAKE_INTERACT_RADIUS       = 4.5f;
const float CAMPFIRE_INTERACT_RADIUS   = 7.0f;
const float TREEHOUSE_INTERACT_RADIUS  = 6.0f;

// ----------------------------------------------------------------fishing ---

const float FISH_BITE_MIN_SECONDS = 2.5f;
const float FISH_BITE_MAX_SECONDS = 6.0f;
const float FISH_COOLDOWN_SECONDS = 4.0f;
const float FISH_REEL_SECONDS     = 1.2f;

// -------------------------------------------------------------- campfire ---

const int MAX_SMOKE_PARTICLES = 60;

// ------------------------------------------------------------------ swing --
const float SWING_X = 10.0f;
const float SWING_Z = 18.0f;
const float SWING_INTERACT_RADIUS   = 2.6f;
const float TREEHOUSE_PLATFORM_RADIUS = 1.7f;

// ----------------------------------------------------------- Sakura hero ----
const float SAKURA_X = -6.2f;
const float SAKURA_Z =  2.2f;
const float SAKURA_SWING_INTERACT_RADIUS = 2.8f;
const float SAKURA_PETAL_RADIUS = 5.2f;

} // namespace Config

#endif // LIVINGISLAND_CONFIG_H