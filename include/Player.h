// ============================================================================
// include/Player.h
// ============================================================================
// ============================================================================
//  Player.h - the visible player avatar the person actually controls.
//
//  Before this file, WASD drove the free-flying Camera directly - the camera
//  WAS the exploration. Player now owns position/velocity and reads
//  WASD/SHIFT/SPACE itself, applies its own gravity and ground/object/world
//  collision, and picks its own animation state (idle/walk/run/jump). Camera
//  no longer moves on its own; Application positions it relative to wherever
//  the player ends up - at the player's eyes in first person, or orbiting
//  behind via Camera::followThirdPerson in third person ("V").
//
//  The avatar's animation and rendering are NOT a new system: Player calls
//  the exact same evaluateHumanoidPose / renderHumanoidSkeleton free
//  functions Character.h exposes, so the player and the NPCs are drawn and
//  animated by one shared piece of code (section 22's "no duplicate system"
//  rule).
//
//  Object collision (trees, the treehouse) is simple circle-vs-circle
//  push-out against World::collisionObstacles() - see
//  resolveObstacleCollisions() below.
// ============================================================================
#ifndef LIVINGISLAND_PLAYER_H
#define LIVINGISLAND_PLAYER_H

#include "Character.h"       // AnimationState, Pose, evaluateHumanoidPose(), renderHumanoidSkeleton()
#include "RenderContext.h"
#include "Utilities.h"

#include <vector>

class Terrain;
class Input;

class Player {
public:
    Player();

    void initialize(const util::Vec3& groundPosition, float facingDegrees);

    // cameraYawDegrees uses the same convention as Camera::yaw() (0 = +X).
    // Movement is relative to where the camera is looking, flattened to the
    // horizontal plane, the way a third-person action game controls - not
    // relative to whichever way the avatar's body currently happens to face.
    // obstacles: tree trunks and the treehouse footprint (World::
    // collisionObstacles()) - simple circle-vs-circle push-out, per section
    // 16 ("simple sphere/circle collision is enough").
    void update(float deltaTime, const Input& input, float cameraYawDegrees,
                const Terrain& terrain, const std::vector<util::Zone>& obstacles);

    void render(const RenderContext& context) const;

    // Instantly relocate the player (used by the "6"/"7" landmark shortcuts).
    // Not a random teleport during play - only ever called from an explicit
    // debug shortcut, same as before.
    void teleportTo(const util::Vec3& groundPosition, float facingDegrees);

    util::Vec3 position() const { return m_position; }
    util::Vec3 eyePosition() const;
    float facing() const { return m_facing; }
    bool  isMoving()   const { return m_isMoving; }
    bool  isRunning()  const { return m_isRunning; }
    bool  isGrounded() const { return m_grounded; }

    // Debug shortcut (bound to "C", same key the old camera used): with this
    // off, the player is no longer snapped to the ground each frame - a
    // cheap way to confirm the collision code is actually doing something.
    bool collideWithGround;

    // --- treehouse platform -------------------------------------------
    // While elevated, gravity and terrain grounding are suspended and the
    // player is pinned to a fixed platform height instead - normal WASD
    // movement still runs, clamped to the platform's footprint.
    void enterElevated(const util::Vec3& center, float platformY, float radius);
    void exitElevated();
    bool isElevated() const { return m_elevated; }

    // --- swing ----------------------------------------------------------
    // Fully bypasses normal movement/gravity: position is driven each frame
    // by whatever Swing::seatPosition() the World reports.
    void beginSwingRide();
    void followSwing(const util::Vec3& seatPosition, float facingDegrees, float deltaTime);
    void endSwingRide(const util::Vec3& groundPosition, float facingDegrees);
    bool ridingSwing() const { return m_ridingSwing; }

private:
    void setState(AnimationState state);
    void resolveObstacleCollisions(const std::vector<util::Zone>& obstacles);

    util::Vec3 m_position;           // feet / ground contact point
    util::Vec3 m_horizontalVelocity;
    float m_verticalVelocity;
    bool  m_grounded;

    float m_facing;
    float m_targetFacing;

    AnimationState m_state;
    float m_animTime;
    Pose  m_blendFrom;
    float m_blendTimer;

    bool m_isMoving;
    bool m_isRunning;

    bool m_elevated;
    util::Vec3 m_elevatedCenter;
    float m_elevatedY;
    float m_elevatedRadius;

    bool m_ridingSwing;

    // A single consistent outfit, distinct from the NPC palette, so the
    // avatar always reads as "you" rather than a tenth villager.
    util::Vec3 m_skinColor;
    util::Vec3 m_shirtColor;
    util::Vec3 m_trouserColor;
    util::Vec3 m_hairColor;
};

#endif // LIVINGISLAND_PLAYER_H