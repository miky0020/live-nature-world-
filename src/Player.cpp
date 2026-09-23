// ============================================================================
// src/Player.cpp
// ============================================================================
#include "Player.h"
#include "Config.h"
#include "GLIncludes.h"
#include "Input.h"
#include "Terrain.h"

using util::Vec3;

Player::Player()
    : collideWithGround(true)
    , m_verticalVelocity(0.0f)
    , m_grounded(true)
    , m_facing(0.0f)
    , m_targetFacing(0.0f)
    , m_state(ANIM_IDLE)
    , m_animTime(0.0f)
    , m_blendTimer(1.0f)
    , m_isMoving(false)
    , m_isRunning(false)
    , m_elevated(false)
    , m_elevatedCenter(0.0f, 0.0f, 0.0f)
    , m_elevatedY(0.0f)
    , m_elevatedRadius(0.0f)
    , m_ridingSwing(false)
    // A calm, consistent outfit - blue shirt, dark trousers - kept apart
    // from the NPC palette in CharacterManager so the avatar never gets
    // mistaken for one of the nine villagers.
    , m_skinColor(0.80f, 0.63f, 0.50f)
    , m_shirtColor(0.22f, 0.42f, 0.78f)
    , m_trouserColor(0.24f, 0.24f, 0.28f)
    , m_hairColor(0.22f, 0.16f, 0.12f)
{
}

void Player::initialize(const Vec3& groundPosition, float facingDegrees) {
    m_position     = groundPosition;
    m_facing       = facingDegrees;
    m_targetFacing = facingDegrees;
    m_blendFrom    = evaluateHumanoidPose(ANIM_IDLE, 0.0f, 0.0f);
}

void Player::teleportTo(const Vec3& groundPosition, float facingDegrees) {
    m_position          = groundPosition;
    m_facing            = facingDegrees;
    m_targetFacing      = facingDegrees;
    m_horizontalVelocity = Vec3(0.0f, 0.0f, 0.0f);
    m_verticalVelocity   = 0.0f;
    m_grounded           = true;
}

Vec3 Player::eyePosition() const {
    return m_position + Vec3(0.0f, Config::PLAYER_EYE_HEIGHT, 0.0f);
}

void Player::setState(AnimationState state) {
    if (state == m_state) return;

    // Capture wherever the body actually is right now and ease from there,
    // exactly like Character::setState - so switching from, say, running to
    // idle mid-stride never snaps.
    m_blendFrom  = evaluateHumanoidPose(m_state, m_animTime, 0.0f);
    m_state      = state;
    m_blendTimer = 0.0f;
}

void Player::update(float deltaTime, const Input& input, float cameraYawDegrees,
                     const Terrain& terrain, const std::vector<util::Zone>& obstacles) {
    m_animTime += deltaTime;

    // --- horizontal movement, relative to where the camera looks ----------
    float yawRad = util::toRadians(cameraYawDegrees);
    Vec3 forward(std::cos(yawRad), 0.0f, std::sin(yawRad));
    Vec3 right = util::cross(forward, Vec3(0.0f, 1.0f, 0.0f)).normalized();

    Vec3 wish(0.0f, 0.0f, 0.0f);
    if (input.key('w')) wish += forward;
    if (input.key('s')) wish -= forward;
    if (input.key('d')) wish += right;
    if (input.key('a')) wish -= right;

    bool wantsMove = wish.lengthSquared() > 1e-6f;
    if (wantsMove) wish = wish.normalized();

    m_isMoving  = wantsMove;
    m_isRunning = wantsMove && input.shift();

    float speed = Config::PLAYER_WALK_SPEED
                * (m_isRunning ? Config::PLAYER_RUN_MULTIPLIER : 1.0f);
    Vec3 targetVelocity = wish * speed;

    // Same frame-rate-independent exponential approach the camera used to
    // use for itself - a little weight so starting/stopping isn't a snap.
    float blend = 1.0f - std::exp(-Config::PLAYER_ACCEL_RATE * deltaTime);
    m_horizontalVelocity += (targetVelocity - m_horizontalVelocity) * blend;
    if (m_horizontalVelocity.lengthSquared() < 1e-6f) {
        m_horizontalVelocity = Vec3(0.0f, 0.0f, 0.0f);
    }

    m_position.x += m_horizontalVelocity.x * deltaTime;
    m_position.z += m_horizontalVelocity.z * deltaTime;

    // Stay inside the island, the same margin style the free camera used.
    float limit = terrain.halfSize() - 2.0f;
    if (m_position.x >  limit) { m_position.x =  limit; m_horizontalVelocity.x = 0.0f; }
    if (m_position.x < -limit) { m_position.x = -limit; m_horizontalVelocity.x = 0.0f; }
    if (m_position.z >  limit) { m_position.z =  limit; m_horizontalVelocity.z = 0.0f; }
    if (m_position.z < -limit) { m_position.z = -limit; m_horizontalVelocity.z = 0.0f; }

    if (collideWithGround) resolveObstacleCollisions(obstacles);

    // --- facing: turn toward the movement direction, not the camera -------
    if (wantsMove) {
        m_targetFacing = util::toDegrees(std::atan2(wish.x, wish.z));
    }
    float difference = m_targetFacing - m_facing;
    while (difference >  180.0f) difference -= 360.0f;
    while (difference < -180.0f) difference += 360.0f;
    m_facing += difference * (1.0f - std::exp(-Config::PLAYER_TURN_RATE * deltaTime));
    m_facing = util::wrapf(m_facing, 360.0f);

    // --- jump / gravity / ground, OR pinned to a platform ------------------
    if (m_elevated) {
        // Clamp to the platform footprint - there's no rail collision, so
        // this is what stops the player from walking off the edge into air.
        float dx = m_position.x - m_elevatedCenter.x;
        float dz = m_position.z - m_elevatedCenter.z;
        float dist = std::sqrt(dx * dx + dz * dz);
        if (dist > m_elevatedRadius && dist > 1e-5f) {
            float scale = m_elevatedRadius / dist;
            m_position.x = m_elevatedCenter.x + dx * scale;
            m_position.z = m_elevatedCenter.z + dz * scale;
        }
        m_position.y = m_elevatedY;
        m_verticalVelocity = 0.0f;
        m_grounded = true;
    } else {
        if (m_grounded && input.key(' ')) {
            m_verticalVelocity = Config::PLAYER_JUMP_SPEED;
            m_grounded = false;
        }

        m_verticalVelocity -= Config::PLAYER_GRAVITY * deltaTime;
        m_position.y += m_verticalVelocity * deltaTime;

        if (collideWithGround) {
            float groundY = terrain.heightAt(m_position.x, m_position.z);
            if (m_position.y <= groundY) {
                m_position.y = groundY;
                m_verticalVelocity = 0.0f;
                m_grounded = true;
            } else {
                m_grounded = false;
            }
        }
    }

    // --- pick the animation the current motion actually calls for ---------
    if (!m_grounded) {
        setState(ANIM_JUMPING);
    } else if (m_isRunning) {
        setState(ANIM_RUNNING);
    } else if (m_isMoving) {
        setState(ANIM_WALKING);
    } else {
        setState(ANIM_IDLE);
    }

    if (m_blendTimer < 1.0f) {
        m_blendTimer += deltaTime / Config::PLAYER_ANIM_BLEND_SECONDS;
        if (m_blendTimer > 1.0f) m_blendTimer = 1.0f;
    }
}

void Player::resolveObstacleCollisions(const std::vector<util::Zone>& obstacles) {    for (size_t i = 0; i < obstacles.size(); ++i) {
        float dx = m_position.x - obstacles[i].center.x;
        float dz = m_position.z - obstacles[i].center.z;
        float minDist = obstacles[i].radius + Config::PLAYER_COLLIDE_RADIUS;
        float distSq = dx * dx + dz * dz;

        if (distSq < minDist * minDist && distSq > 1e-8f) {
            float dist = std::sqrt(distSq);
            float push = minDist - dist;
            m_position.x += (dx / dist) * push;
            m_position.z += (dz / dist) * push;
        }
    }
}

void Player::render(const RenderContext& context) const {
    Vec3 centre = m_position + Vec3(0.0f, 0.85f, 0.0f);
    if (!context.frustum.sphereVisible(centre, 1.6f)) return;

    Pose target = evaluateHumanoidPose(m_state, m_animTime, 0.0f);
    Pose pose = target;
    if (m_blendTimer < 1.0f) {
        float t = m_blendTimer * m_blendTimer * (3.0f - 2.0f * m_blendTimer);
        pose = lerpPose(m_blendFrom, target, t);
    }

    glPushMatrix();
    glTranslatef(m_position.x, m_position.y, m_position.z);
    glRotatef(m_facing, 0.0f, 1.0f, 0.0f);
    renderHumanoidSkeleton(pose, /*lod=*/0, m_skinColor, m_shirtColor,
                            m_trouserColor, m_hairColor);
    glPopMatrix();
}
// ---------------------------------------------------------------------------
//  Treehouse platform
// ---------------------------------------------------------------------------
void Player::enterElevated(const Vec3& center, float platformY, float radius) {
    m_elevated       = true;
    m_elevatedCenter = center;
    m_elevatedY      = platformY;
    m_elevatedRadius = radius;
    m_verticalVelocity = 0.0f;
    m_grounded = true;
    m_position.y = platformY;
}

void Player::exitElevated() {
    m_elevated = false;
    // Gravity takes back over next update() - the player falls to whatever
    // ground is actually below wherever they were standing on the platform.
}

// ---------------------------------------------------------------------------
//  Swing
// ---------------------------------------------------------------------------
void Player::beginSwingRide() {
    m_ridingSwing = true;
    m_horizontalVelocity = Vec3(0.0f, 0.0f, 0.0f);
    m_verticalVelocity   = 0.0f;
    m_grounded = true;
    setState(ANIM_SITTING);
}

void Player::followSwing(const Vec3& seatPosition, float facingDegrees, float deltaTime) {
    m_position     = seatPosition;
    m_facing       = facingDegrees;
    m_targetFacing = facingDegrees;
    m_animTime += deltaTime;
    if (m_blendTimer < 1.0f) {
        m_blendTimer += deltaTime / Config::PLAYER_ANIM_BLEND_SECONDS;
        if (m_blendTimer > 1.0f) m_blendTimer = 1.0f;
    }
}

void Player::endSwingRide(const Vec3& groundPosition, float facingDegrees) {
    m_ridingSwing  = false;
    m_position     = groundPosition;
    m_facing       = facingDegrees;
    m_targetFacing = facingDegrees;
    m_grounded     = true;
    setState(ANIM_IDLE);
}
