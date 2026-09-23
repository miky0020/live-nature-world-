// ============================================================================
// src/Camera.cpp
// ============================================================================
#include "Camera.h"
#include "Config.h"
#include "GLIncludes.h"
#include "Terrain.h"

using util::Vec3;

Camera::Camera()
    : sensitivity(Config::CAM_SENSITIVITY)
    , fieldOfView(Config::CAM_FOV)
    , nearPlane(Config::CAM_NEAR_PLANE)
    , farPlane(Config::CAM_FAR_PLANE)
    , m_position(Config::CAM_START_X, Config::CAM_START_Y, Config::CAM_START_Z)
    , m_yaw(215.0f)
    , m_pitch(-12.0f)
{
}

void Camera::setPosition(const Vec3& position) { m_position = position; }

void Camera::lookAt(const Vec3& target) {
    Vec3 dir = (target - m_position).normalized();
    if (dir.lengthSquared() < 1e-6f) return;
    m_yaw   = util::toDegrees(std::atan2(dir.z, dir.x));
    m_pitch = util::toDegrees(std::asin(util::clampf(dir.y, -1.0f, 1.0f)));
}

void Camera::addLookDelta(float dxPixels, float dyPixels) {
    m_yaw   = util::wrapf(m_yaw + dxPixels * sensitivity, 360.0f);
    m_pitch = util::clampf(m_pitch - dyPixels * sensitivity, -89.0f, 89.0f);
}

void Camera::followThirdPerson(float deltaTime, const Vec3& targetFeetPosition,
                                const Terrain& terrain, bool snap) {
    Vec3 anchor = targetFeetPosition + Vec3(0.0f, Config::THIRD_PERSON_HEIGHT, 0.0f);
    Vec3 desired = anchor - forward() * Config::THIRD_PERSON_DISTANCE;

    // Keep the lens a little clear of whatever ground sits under it, so a
    // hillside directly behind the player can't put the camera underground.
    float groundY = terrain.heightAt(desired.x, desired.z) + 0.35f;
    if (desired.y < groundY) desired.y = groundY;

    if (snap) {
        m_position = desired;
    } else {
        float blend = 1.0f - std::exp(-Config::THIRD_PERSON_DAMP_RATE * deltaTime);
        m_position += (desired - m_position) * blend;
    }
}

Vec3 Camera::forward() const {
    float cy = std::cos(util::toRadians(m_yaw));
    float sy = std::sin(util::toRadians(m_yaw));
    float cp = std::cos(util::toRadians(m_pitch));
    float sp = std::sin(util::toRadians(m_pitch));
    return Vec3(cy * cp, sp, sy * cp).normalized();
}

Vec3 Camera::right() const {
    return util::cross(forward(), Vec3(0.0f, 1.0f, 0.0f)).normalized();
}

Vec3 Camera::up() const {
    return util::cross(right(), forward()).normalized();
}

void Camera::applyProjection(int windowWidth, int windowHeight) const {
    if (windowHeight <= 0) windowHeight = 1;
    float aspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(static_cast<GLdouble>(fieldOfView),
                   static_cast<GLdouble>(aspect),
                   static_cast<GLdouble>(nearPlane),
                   static_cast<GLdouble>(farPlane));
    glMatrixMode(GL_MODELVIEW);
}

void Camera::applyView() const {
    Vec3 target = m_position + forward();
    Vec3 u = up();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(m_position.x, m_position.y, m_position.z,
              target.x,     target.y,     target.z,
              u.x,          u.y,          u.z);
}