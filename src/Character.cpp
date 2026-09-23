// ============================================================================
// src/Character.cpp
// ============================================================================
#include "Character.h"
#include "Config.h"
#include "GLIncludes.h"
#include "Primitives.h"
#include "Terrain.h"

using util::Vec3;

// =============================================================== Pose =======
Pose::Pose()
    : hipHeight(0.0f), bodyLean(0.0f), bodyTwist(0.0f), bodyRoll(0.0f)
    , headPitch(0.0f), headYaw(0.0f)
    , leftArmPitch(0.0f),  leftArmSwing(6.0f),  leftElbow(10.0f)
    , rightArmPitch(0.0f), rightArmSwing(6.0f), rightElbow(10.0f)
    , leftLegPitch(0.0f),  leftKnee(0.0f)
    , rightLegPitch(0.0f), rightKnee(0.0f)
{
}

Pose lerpPose(const Pose& a, const Pose& b, float t) {
    Pose out;
    out.hipHeight     = util::lerpf(a.hipHeight,     b.hipHeight,     t);
    out.bodyLean      = util::lerpf(a.bodyLean,      b.bodyLean,      t);
    out.bodyTwist     = util::lerpf(a.bodyTwist,     b.bodyTwist,     t);
    out.bodyRoll      = util::lerpf(a.bodyRoll,      b.bodyRoll,      t);
    out.headPitch     = util::lerpf(a.headPitch,     b.headPitch,     t);
    out.headYaw       = util::lerpf(a.headYaw,       b.headYaw,       t);
    out.leftArmPitch  = util::lerpf(a.leftArmPitch,  b.leftArmPitch,  t);
    out.leftArmSwing  = util::lerpf(a.leftArmSwing,  b.leftArmSwing,  t);
    out.leftElbow     = util::lerpf(a.leftElbow,     b.leftElbow,     t);
    out.rightArmPitch = util::lerpf(a.rightArmPitch, b.rightArmPitch, t);
    out.rightArmSwing = util::lerpf(a.rightArmSwing, b.rightArmSwing, t);
    out.rightElbow    = util::lerpf(a.rightElbow,    b.rightElbow,    t);
    out.leftLegPitch  = util::lerpf(a.leftLegPitch,  b.leftLegPitch,  t);
    out.leftKnee      = util::lerpf(a.leftKnee,      b.leftKnee,      t);
    out.rightLegPitch = util::lerpf(a.rightLegPitch, b.rightLegPitch, t);
    out.rightKnee     = util::lerpf(a.rightKnee,     b.rightKnee,     t);
    return out;
}

// ========================================================== Character =======
Character::Character()
    : m_hasLookTarget(false)
    , m_facing(0.0f)
    , m_targetFacing(0.0f)
    , m_groundHeight(0.0f)
    , m_state(ANIM_IDLE)
    , m_animTime(0.0f)
    , m_phase(0.0f)
    , m_speedScale(1.0f)
    , m_blendTimer(1.0f)
    , m_blendDuration(0.40f)
    , m_currentWaypoint(0)
    , m_walkSpeed(1.5f)
    , m_heightScale(1.0f)
    , m_buildScale(1.0f)
{
}

void Character::initialize(const Vec3& position, float facingDegrees,
                           AnimationState state, unsigned seed)
{
    util::Rng rng(seed);

    m_position     = position;
    m_groundHeight = position.y;
    m_facing       = facingDegrees;
    m_targetFacing = facingDegrees;
    m_state        = state;
    m_animTime     = rng.range(0.0f, 10.0f);
    m_phase        = rng.range(0.0f, util::TWO_PI);
    m_speedScale   = rng.range(0.86f, 1.18f);
    m_blendTimer   = 1.0f;
    m_blendFrom    = evaluateHumanoidPose(state, m_animTime, m_phase);

    m_heightScale = rng.range(0.90f, 1.10f);
    m_buildScale  = rng.range(0.90f, 1.12f);
    m_walkSpeed   = rng.range(1.25f, 1.85f);

    // A small palette of skin tones, and freely varied clothing so a crowd of
    // nine never reads as nine copies.
    const Vec3 skinTones[5] = {
        Vec3(0.86f, 0.68f, 0.54f), Vec3(0.72f, 0.52f, 0.38f),
        Vec3(0.55f, 0.38f, 0.27f), Vec3(0.38f, 0.26f, 0.19f),
        Vec3(0.92f, 0.78f, 0.66f)
    };
    m_skinColor = skinTones[rng.rangeInt(0, 4)];

    const Vec3 hairTones[4] = {
        Vec3(0.10f, 0.08f, 0.07f), Vec3(0.28f, 0.18f, 0.10f),
        Vec3(0.52f, 0.38f, 0.18f), Vec3(0.34f, 0.30f, 0.28f)
    };
    m_hairColor = hairTones[rng.rangeInt(0, 3)];

    m_shirtColor = Vec3(rng.range(0.20f, 0.90f),
                        rng.range(0.20f, 0.85f),
                        rng.range(0.20f, 0.90f));
    m_trouserColor = Vec3(rng.range(0.15f, 0.45f),
                          rng.range(0.15f, 0.42f),
                          rng.range(0.20f, 0.55f));
}

void Character::setState(AnimationState state) {
    if (state == m_state) return;

    // Capture where the body is right now, and ease from there. Without this
    // the character would teleport between poses.
    m_blendFrom  = evaluateHumanoidPose(m_state, m_animTime, m_phase);
    m_state      = state;
    m_blendTimer = 0.0f;
}

void Character::setPatrol(const std::vector<Vec3>& waypoints) {
    m_waypoints = waypoints;
    m_currentWaypoint = 0;
}

void Character::faceToward(const Vec3& target) {
    Vec3 delta = target - m_position;
    if (delta.lengthSquared() < 1e-4f) return;
    m_targetFacing = util::toDegrees(std::atan2(delta.x, delta.z));
}

void Character::updatePatrol(float deltaTime, const Terrain& terrain) {
    if (m_waypoints.empty()) return;

    Vec3 target = m_waypoints[static_cast<size_t>(m_currentWaypoint)];
    Vec3 delta(target.x - m_position.x, 0.0f, target.z - m_position.z);
    float distance = delta.length();

    if (distance < 0.6f) {
        m_currentWaypoint = (m_currentWaypoint + 1)
                          % static_cast<int>(m_waypoints.size());
        return;
    }

    Vec3 direction = delta / distance;
    m_position.x += direction.x * m_walkSpeed * deltaTime;
    m_position.z += direction.z * m_walkSpeed * deltaTime;

    // Follow the ground rather than floating over it.
    m_groundHeight = terrain.heightAt(m_position.x, m_position.z);
    m_position.y = m_groundHeight;

    m_targetFacing = util::toDegrees(std::atan2(direction.x, direction.z));
}

void Character::update(float deltaTime, const Terrain& terrain) {
    m_animTime += deltaTime * m_speedScale;

    if (m_state == ANIM_WALKING) {
        updatePatrol(deltaTime, terrain);
    } else {
        m_groundHeight = terrain.heightAt(m_position.x, m_position.z);
        m_position.y = m_groundHeight;
    }

    if (m_hasLookTarget) faceToward(m_lookTarget);

    // Turn smoothly, taking the short way round the circle.
    float difference = m_targetFacing - m_facing;
    while (difference >  180.0f) difference -= 360.0f;
    while (difference < -180.0f) difference += 360.0f;
    m_facing += difference * (1.0f - std::exp(-6.0f * deltaTime));
    m_facing = util::wrapf(m_facing, 360.0f);

    if (m_blendTimer < 1.0f) {
        m_blendTimer += deltaTime / m_blendDuration;
        if (m_blendTimer > 1.0f) m_blendTimer = 1.0f;
    }
}

// ---------------------------------------------------------------------------
//  The animation library. One function, one case per state.
// ---------------------------------------------------------------------------
Pose evaluateHumanoidPose(AnimationState state, float time, float phase) {
    Pose pose;
    float t = time + phase;

    // ----------------------------------------------------------------------
    //  SIGN CONVENTION (matters - the original build had legs bending the
    //  wrong way because this was never pinned down):
    //
    //    limb pitch  : POSITIVE swings the limb FORWARD  (+Z, the way the
    //                  character faces)
    //    knee        : POSITIVE folds the shin BACKWARD, like a real knee
    //    elbow       : POSITIVE folds the forearm FORWARD, like a real elbow
    //    bodyLean    : POSITIVE leans the chest forward
    //    headPitch   : POSITIVE tips the chin down
    //
    //  drawSkeleton() applies the negations; every pose below is written in
    //  these human terms.
    // ----------------------------------------------------------------------

    switch (state) {
        case ANIM_IDLE: {
            float breathe = std::sin(t * 1.5f);
            pose.hipHeight = breathe * 0.012f;
            pose.bodyLean  = 1.0f + breathe * 1.2f;
            pose.bodyRoll  = std::sin(t * 0.45f) * 1.8f;
            pose.headYaw   = std::sin(t * 0.33f) * 16.0f
                           + std::sin(t * 0.11f) * 9.0f;
            pose.headPitch = std::sin(t * 0.7f) * 3.0f;

            pose.leftArmPitch  = std::sin(t * 0.9f) * 3.0f;
            pose.rightArmPitch = std::sin(t * 0.9f + 1.0f) * 3.0f;
            pose.leftArmSwing  = 5.0f + breathe * 1.2f;
            pose.rightArmSwing = 5.0f - breathe * 1.2f;
            pose.leftElbow     = 8.0f;
            pose.rightElbow    = 8.0f;
            break;
        }

        case ANIM_WALKING: {
            float cycle = t * 4.6f;
            float swing = std::sin(cycle);

            // Legs swing in opposition, arms counter-swing to the legs.
            pose.leftLegPitch  =  swing * 32.0f;
            pose.rightLegPitch = -swing * 32.0f;

            // A knee only folds while its leg is travelling backward. This is
            // the detail that separates a walk from a shuffle.
            pose.leftKnee  = util::clampf(-swing, 0.0f, 1.0f) * 48.0f;
            pose.rightKnee = util::clampf( swing, 0.0f, 1.0f) * 48.0f;

            pose.leftArmPitch  = -swing * 26.0f;
            pose.rightArmPitch =  swing * 26.0f;
            pose.leftElbow  = 14.0f + std::fabs(swing) * 12.0f;
            pose.rightElbow = 14.0f + std::fabs(swing) * 12.0f;
            pose.leftArmSwing  = 5.0f;
            pose.rightArmSwing = 5.0f;

            // Vertical bob runs at twice the stride frequency.
            pose.hipHeight = std::fabs(std::sin(cycle)) * 0.05f - 0.02f;
            pose.bodyLean  = 4.0f;
            pose.bodyTwist = -swing * 5.0f;
            pose.bodyRoll  = std::sin(cycle) * 2.0f;
            pose.headYaw   = std::sin(t * 0.6f) * 8.0f;
            break;
        }

        case ANIM_RUNNING: {
            // Same walk-cycle shape as ANIM_WALKING, but faster, with a much
            // bigger stride and more forward lean - this is what actually
            // reads as running rather than a fast power-walk.
            float cycle = t * 8.0f;
            float swing = std::sin(cycle);

            pose.leftLegPitch  =  swing * 52.0f;
            pose.rightLegPitch = -swing * 52.0f;
            pose.leftKnee  = 8.0f + util::clampf(-swing, 0.0f, 1.0f) * 78.0f;
            pose.rightKnee = 8.0f + util::clampf( swing, 0.0f, 1.0f) * 78.0f;

            pose.leftArmPitch  = -swing * 46.0f;
            pose.rightArmPitch =  swing * 46.0f;
            pose.leftElbow  = 62.0f + std::fabs(swing) * 18.0f;
            pose.rightElbow = 62.0f + std::fabs(swing) * 18.0f;
            pose.leftArmSwing  = 4.0f;
            pose.rightArmSwing = 4.0f;

            pose.hipHeight = std::fabs(std::sin(cycle)) * 0.09f - 0.01f;
            pose.bodyLean  = 12.0f;
            pose.bodyTwist = -swing * 8.0f;
            pose.bodyRoll  = std::sin(cycle) * 3.0f;
            pose.headPitch = 4.0f;
            break;
        }

        case ANIM_JUMPING: {
            // A held "airborne" shape rather than a strict cycle: knees
            // tucked up, arms swung up and back for momentum. The gentle
            // sine keeps it reading as alive rather than a frozen frame
            // while the player is in the air.
            float wobble = std::sin(t * 5.0f);
            pose.hipHeight = 0.10f;
            pose.bodyLean  = 10.0f + wobble * 2.0f;

            pose.leftLegPitch  = -18.0f;
            pose.rightLegPitch = -18.0f;
            pose.leftKnee  = 58.0f + wobble * 6.0f;
            pose.rightKnee = 58.0f + wobble * 6.0f;

            pose.leftArmPitch  = 132.0f;
            pose.rightArmPitch = 132.0f;
            pose.leftArmSwing  = 20.0f;
            pose.rightArmSwing = 20.0f;
            pose.leftElbow  = 30.0f;
            pose.rightElbow = 30.0f;

            pose.headPitch = -6.0f;
            break;
        }

        case ANIM_SITTING: {
            // Hips drop onto the seat, thighs go FORWARD, shins drop down.
            pose.hipHeight = -0.22f + std::sin(t * 1.1f) * 0.006f;
            pose.bodyLean  = 6.0f + std::sin(t * 0.9f) * 2.0f;

            pose.leftLegPitch  = 84.0f;
            pose.rightLegPitch = 81.0f;
            pose.leftKnee      = 84.0f;
            pose.rightKnee     = 87.0f;

            pose.leftArmPitch  = 22.0f + std::sin(t * 0.8f) * 4.0f;
            pose.rightArmPitch = 20.0f + std::sin(t * 0.7f + 2.0f) * 4.0f;
            pose.leftElbow     = 38.0f;
            pose.rightElbow    = 42.0f;
            pose.leftArmSwing  = 9.0f;
            pose.rightArmSwing = 9.0f;

            pose.headYaw   = std::sin(t * 0.5f) * 12.0f;
            pose.headPitch = 4.0f + std::sin(t * 0.8f) * 3.0f;
            break;
        }

        case ANIM_TALKING: {
            // Sitting, but gesturing. The gesture rate is deliberately
            // irregular - two frequencies rather than one.
            pose.hipHeight = -0.22f;
            pose.bodyLean  = 7.0f + std::sin(t * 1.3f) * 3.0f;

            pose.leftLegPitch  = 84.0f;
            pose.rightLegPitch = 81.0f;
            pose.leftKnee      = 84.0f;
            pose.rightKnee     = 87.0f;

            float gesture = std::sin(t * 3.1f) * 0.6f + std::sin(t * 5.3f) * 0.4f;
            pose.rightArmPitch = 40.0f + gesture * 26.0f;
            pose.rightElbow    = 58.0f + gesture * 22.0f;
            pose.rightArmSwing = 15.0f + gesture * 8.0f;
            pose.leftArmPitch  = 26.0f + std::sin(t * 2.2f) * 10.0f;
            pose.leftElbow     = 46.0f;
            pose.leftArmSwing  = 11.0f;

            pose.headPitch = std::sin(t * 2.6f) * 7.0f;
            pose.headYaw   = std::sin(t * 1.1f) * 9.0f;
            pose.bodyTwist = std::sin(t * 1.4f) * 5.0f;
            break;
        }

        case ANIM_DANCING: {
            float beat = t * 3.4f;
            pose.hipHeight = std::fabs(std::sin(beat)) * 0.15f;
            pose.bodyTwist = std::sin(beat * 0.5f) * 30.0f;
            pose.bodyRoll  = std::sin(beat) * 10.0f;
            pose.bodyLean  = std::sin(beat * 0.5f + 1.0f) * 6.0f;

            // Arms raised overhead and pumping.
            pose.leftArmPitch  = 130.0f + std::sin(beat) * 40.0f;
            pose.rightArmPitch = 130.0f + std::sin(beat + util::PI) * 40.0f;
            pose.leftArmSwing  = 32.0f + std::sin(beat * 0.5f) * 18.0f;
            pose.rightArmSwing = 32.0f - std::sin(beat * 0.5f) * 18.0f;
            pose.leftElbow  = 40.0f + std::sin(beat + 1.0f) * 25.0f;
            pose.rightElbow = 40.0f + std::sin(beat) * 25.0f;

            pose.leftLegPitch  =  std::sin(beat) * 16.0f;
            pose.rightLegPitch = -std::sin(beat) * 16.0f;
            pose.leftKnee  = 10.0f + util::clampf(-std::sin(beat), 0.0f, 1.0f) * 30.0f;
            pose.rightKnee = 10.0f + util::clampf( std::sin(beat), 0.0f, 1.0f) * 30.0f;

            pose.headYaw   = std::sin(beat * 0.5f) * 20.0f;
            pose.headPitch = std::sin(beat) * 8.0f;
            break;
        }

        case ANIM_WAVING: {
            float wave = std::sin(t * 6.2f);

            pose.rightArmPitch = 152.0f;              // raised overhead
            pose.rightArmSwing = 24.0f + wave * 16.0f; // the wave itself
            pose.rightElbow    = 28.0f + wave * 20.0f;

            pose.leftArmPitch = std::sin(t * 1.1f) * 5.0f;
            pose.leftArmSwing = 6.0f;
            pose.leftElbow    = 10.0f;

            pose.hipHeight = std::sin(t * 1.5f) * 0.012f;
            pose.bodyRoll  = wave * 2.5f;
            pose.headYaw   = wave * 6.0f;
            pose.headPitch = -3.0f;
            break;
        }

        case ANIM_CHEERING: {
            float bounce = std::fabs(std::sin(t * 4.0f));
            pose.hipHeight = bounce * 0.12f;

            pose.leftArmPitch  = 162.0f + std::sin(t * 4.0f) * 12.0f;
            pose.rightArmPitch = 162.0f + std::sin(t * 4.0f + 0.4f) * 12.0f;
            pose.leftArmSwing  = 26.0f;
            pose.rightArmSwing = 26.0f;
            pose.leftElbow  = 14.0f;
            pose.rightElbow = 14.0f;

            pose.bodyLean  = -5.0f;
            pose.headPitch = -12.0f;
            pose.leftKnee  = bounce * 18.0f;
            pose.rightKnee = bounce * 18.0f;
            break;
        }

        default: break;
    }

    return pose;
}

// ---------------------------------------------------------------------------
//  Blocky character rendering.
//
//  Every part is an axis-aligned box, assembled through a proper joint
//  hierarchy: hip -> thigh -> knee -> shin -> foot, and shoulder -> upper arm
//  -> elbow -> forearm -> hand. Each limb is drawn hanging DOWNWARD from its
//  pivot, so rotating the pivot swings the whole chain the way a real joint
//  does.
// ---------------------------------------------------------------------------
void renderHumanoidSkeleton(const Pose& pose, int lod,
                             const Vec3& skinColor, const Vec3& shirtColor,
                             const Vec3& trouserColor, const Vec3& hairColor) {
    // Proportions, in metres. Roughly 1.65 m tall overall.
    const float thighLen = 0.32f, shinLen  = 0.30f;
    const float legW     = 0.24f, legD     = 0.26f;
    const float torsoH   = 0.60f, torsoW   = 0.56f, torsoD = 0.30f;
    const float neckH    = 0.11f, neckW    = 0.18f;
    const float headW    = 0.38f, headH    = 0.34f, headD  = 0.34f;
    const float upperArm = 0.30f, foreArm  = 0.28f;
    const float armW     = 0.20f, armD     = 0.22f;

    const float hipY = (thighLen + shinLen) + pose.hipHeight;

    Vec3 shoe = trouserColor * 0.55f;

    // ------------------------------------------------------------ legs ----
    for (int side = 0; side < 2; ++side) {
        float sign  = (side == 0) ? -1.0f : 1.0f;
        float pitch = (side == 0) ? pose.leftLegPitch : pose.rightLegPitch;
        float knee  = (side == 0) ? pose.leftKnee     : pose.rightKnee;

        glPushMatrix();
        glTranslatef(sign * 0.14f, hipY, 0.0f);
        glRotatef(-pitch, 1.0f, 0.0f, 0.0f);   // + pitch swings the leg forward

        glColor3f(trouserColor.x, trouserColor.y, trouserColor.z);
        glPushMatrix();
        glTranslatef(0.0f, -thighLen * 0.5f, 0.0f);
        prim::drawBox(legW, thighLen, legD);
        glPopMatrix();

        // knee joint
        glTranslatef(0.0f, -thighLen, 0.0f);
        glRotatef(knee, 1.0f, 0.0f, 0.0f);     // + knee folds the shin backward

        glPushMatrix();
        glTranslatef(0.0f, -shinLen * 0.5f, 0.0f);
        prim::drawBox(legW * 0.94f, shinLen, legD * 0.94f);
        glPopMatrix();

        // foot, extending forward from the ankle
        glTranslatef(0.0f, -shinLen, 0.0f);
        glColor3f(shoe.x, shoe.y, shoe.z);
        glPushMatrix();
        glTranslatef(0.0f, -0.045f, 0.055f);
        prim::drawBox(legW, 0.09f, legD + 0.12f);
        glPopMatrix();

        glPopMatrix();
    }

    // ----------------------------------------------------------- torso ----
    glPushMatrix();
    glTranslatef(0.0f, hipY, 0.0f);
    glRotatef(pose.bodyTwist, 0.0f, 1.0f, 0.0f);
    glRotatef(-pose.bodyLean, 1.0f, 0.0f, 0.0f);
    glRotatef(pose.bodyRoll, 0.0f, 0.0f, 1.0f);

    glColor3f(shirtColor.x, shirtColor.y, shirtColor.z);
    glPushMatrix();
    glTranslatef(0.0f, torsoH * 0.5f, 0.0f);
    prim::drawBox(torsoW, torsoH, torsoD);
    glPopMatrix();

    // ------------------------------------------------------------ arms ----
    const float shoulderY = torsoH - 0.05f;
    for (int side = 0; side < 2; ++side) {
        float sign  = (side == 0) ? -1.0f : 1.0f;
        float pitch = (side == 0) ? pose.leftArmPitch : pose.rightArmPitch;
        float swing = (side == 0) ? pose.leftArmSwing : pose.rightArmSwing;
        float elbow = (side == 0) ? pose.leftElbow    : pose.rightElbow;

        glPushMatrix();
        glTranslatef(sign * (torsoW * 0.5f + armW * 0.5f), shoulderY, 0.0f);
        glRotatef(sign * swing, 0.0f, 0.0f, 1.0f);   // lifts the arm outward
        glRotatef(-pitch, 1.0f, 0.0f, 0.0f);         // + pitch swings forward

        glColor3f(shirtColor.x, shirtColor.y, shirtColor.z);
        glPushMatrix();
        glTranslatef(0.0f, -upperArm * 0.5f, 0.0f);
        prim::drawBox(armW, upperArm, armD);
        glPopMatrix();

        // elbow joint
        glTranslatef(0.0f, -upperArm, 0.0f);
        glRotatef(-elbow, 1.0f, 0.0f, 0.0f);   // + elbow folds forearm forward

        glColor3f(skinColor.x, skinColor.y, skinColor.z);
        glPushMatrix();
        glTranslatef(0.0f, -foreArm * 0.5f, 0.0f);
        prim::drawBox(armW * 0.92f, foreArm, armD * 0.92f);
        glPopMatrix();

        // hand
        glTranslatef(0.0f, -foreArm, 0.0f);
        glPushMatrix();
        glTranslatef(0.0f, -0.05f, 0.0f);
        prim::drawBox(armW * 0.96f, 0.10f, armD * 0.96f);
        glPopMatrix();

        glPopMatrix();
    }

    // ------------------------------------------------------------ neck ----
    // The old build had the head sitting straight on the shoulders with
    // nothing between them, which read as headless from a distance.
    glColor3f(skinColor.x, skinColor.y, skinColor.z);
    glPushMatrix();
    glTranslatef(0.0f, torsoH + neckH * 0.5f, 0.0f);
    prim::drawBox(neckW, neckH, neckW);
    glPopMatrix();

    // ------------------------------------------------------------ head ----
    glPushMatrix();
    glTranslatef(0.0f, torsoH + neckH, 0.0f);
    glRotatef(pose.headYaw, 0.0f, 1.0f, 0.0f);
    glRotatef(-pose.headPitch, 1.0f, 0.0f, 0.0f);

    glColor3f(skinColor.x, skinColor.y, skinColor.z);
    glPushMatrix();
    glTranslatef(0.0f, headH * 0.5f, 0.0f);
    prim::drawBox(headW, headH, headD);
    glPopMatrix();

    // hair: a slab capping the skull
    glColor3f(hairColor.x, hairColor.y, hairColor.z);
    glPushMatrix();
    glTranslatef(0.0f, headH - 0.025f, -0.012f);
    prim::drawBox(headW * 1.03f, 0.10f, headD * 1.03f);
    glPopMatrix();

    if (lod == 0) {
        // Eyes on the front face, so the head has an obvious facing direction.
        glColor3f(0.10f, 0.09f, 0.09f);
        for (int side = 0; side < 2; ++side) {
            float sign = (side == 0) ? -1.0f : 1.0f;
            glPushMatrix();
            glTranslatef(sign * 0.085f, headH * 0.60f, headD * 0.5f + 0.008f);
            prim::drawBox(0.065f, 0.075f, 0.02f);
            glPopMatrix();
        }
    }
    glPopMatrix();

    glPopMatrix();   // torso
}

void Character::render(const RenderContext& context) const {
    Vec3 centre = m_position + Vec3(0.0f, 0.85f, 0.0f);
    if (!context.frustum.sphereVisible(centre, 1.6f)) return;

    float distance = util::distanceXZ(m_position, context.cameraPosition);
    if (distance > context.drawDistance) return;

    ++context.visibleCharacters;

    int lod = (distance < 22.0f) ? 0 : 1;

    // Blend from the pose held at the moment of the last state change.
    Pose target = evaluateHumanoidPose(m_state, m_animTime, m_phase);
    Pose pose = target;
    if (m_blendTimer < 1.0f) {
        float t = m_blendTimer * m_blendTimer * (3.0f - 2.0f * m_blendTimer);
        pose = lerpPose(m_blendFrom, target, t);
    }

    glPushMatrix();
    glTranslatef(m_position.x, m_position.y, m_position.z);
    glRotatef(m_facing, 0.0f, 1.0f, 0.0f);
    glScalef(m_buildScale, m_heightScale, m_buildScale);
    renderHumanoidSkeleton(pose, lod, m_skinColor, m_shirtColor, m_trouserColor, m_hairColor);
    glPopMatrix();
}