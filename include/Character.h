// ============================================================================
// include/Character.h
// ============================================================================
// ============================================================================
//  Character.h - low-poly people and their animation system.
//
//  The design that keeps section 18's "do not duplicate animation code per
//  character" honest:
//
//    * a Pose is a plain bag of joint angles
//    * each animation state is a pure function (state, time, phase) -> Pose
//    * a character stores only its state, its clock and its personal phase
//    * switching state captures the current pose and blends toward the new one
//      over a fraction of a second, so nothing ever snaps (section 17)
//
//  One skeleton drawing routine consumes a Pose. Adding an animation means
//  adding a case to evaluateHumanoidPose - never touching the renderer.
//
//  The pose evaluator and the skeleton renderer are free functions
//  (evaluateHumanoidPose / renderHumanoidSkeleton) rather than private
//  Character methods, precisely so Player (Player.h) can share this exact
//  animation code for the player avatar instead of a second copy - the same
//  "don't duplicate a system that already exists" rule the project follows
//  for cameras, weather, etc., just applied one level lower. Character's
//  public interface and behaviour are unchanged by this.
// ============================================================================
#ifndef LIVINGISLAND_CHARACTER_H
#define LIVINGISLAND_CHARACTER_H

#include "RenderContext.h"
#include "Utilities.h"

#include <vector>

class Terrain;

enum AnimationState {
    ANIM_IDLE = 0,
    ANIM_WALKING,
    ANIM_SITTING,
    ANIM_TALKING,
    ANIM_DANCING,
    ANIM_WAVING,
    ANIM_CHEERING,
    ANIM_RUNNING,   // player avatar today; free for NPCs to use later
    ANIM_JUMPING,   // player avatar today; free for NPCs to use later
    ANIM_STATE_COUNT
};

// Every angle is in degrees. Positive arm/leg pitch swings forward.
struct Pose {
    float hipHeight;     // vertical offset of the whole body
    float bodyLean;      // forward/back tilt
    float bodyTwist;     // rotation about Y
    float bodyRoll;      // side-to-side tilt

    float headPitch;
    float headYaw;

    float leftArmPitch,  leftArmSwing,  leftElbow;
    float rightArmPitch, rightArmSwing, rightElbow;

    float leftLegPitch,  leftKnee;
    float rightLegPitch, rightKnee;

    Pose();
};

Pose lerpPose(const Pose& a, const Pose& b, float t);

// The shared animation library: one pure function, one case per state. This
// is what used to be the private Character::evaluatePose - now free so
// Player can call it too. Adding an animation still means adding one case
// here, in exactly one place, shared by NPCs and the player.
Pose evaluateHumanoidPose(AnimationState state, float time, float phase);

// The shared skeleton renderer, likewise promoted out of Character so both
// Character and Player draw from one hierarchical humanoid builder (hip ->
// legs, torso -> arms, neck -> head) instead of two. lod 0 = full detail
// (draws the eyes), lod 1 = the simplified far-distance version.
void renderHumanoidSkeleton(const Pose& pose, int lod,
                             const util::Vec3& skinColor,
                             const util::Vec3& shirtColor,
                             const util::Vec3& trouserColor,
                             const util::Vec3& hairColor);

class Character {
public:
    Character();

    void initialize(const util::Vec3& position, float facingDegrees,
                    AnimationState state, unsigned seed);

    void setState(AnimationState state);
    AnimationState state() const { return m_state; }

    void setPatrol(const std::vector<util::Vec3>& waypoints);
    void setFacing(float degrees) { m_facing = degrees; }
    void faceToward(const util::Vec3& target);

    void update(float deltaTime, const Terrain& terrain);
    void render(const RenderContext& context) const;

    util::Vec3 position() const { return m_position; }
    float phase() const { return m_phase; }

    // Used by the conversation logic to point people at each other.
    void setLookTarget(const util::Vec3& target) { m_lookTarget = target; m_hasLookTarget = true; }
    void clearLookTarget() { m_hasLookTarget = false; }

private:
    void updatePatrol(float deltaTime, const Terrain& terrain);

    util::Vec3 m_position;
    util::Vec3 m_lookTarget;
    bool  m_hasLookTarget;

    float m_facing;          // degrees about Y
    float m_targetFacing;
    float m_groundHeight;

    AnimationState m_state;
    float m_animTime;
    float m_phase;           // personal offset so no two people are in sync
    float m_speedScale;

    Pose  m_blendFrom;
    float m_blendTimer;
    float m_blendDuration;

    // patrol
    std::vector<util::Vec3> m_waypoints;
    int   m_currentWaypoint;
    float m_walkSpeed;

    // appearance
    util::Vec3 m_skinColor;
    util::Vec3 m_shirtColor;
    util::Vec3 m_trouserColor;
    util::Vec3 m_hairColor;
    float m_heightScale;
    float m_buildScale;
};

#endif // LIVINGISLAND_CHARACTER_H