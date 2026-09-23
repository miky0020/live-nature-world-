// ============================================================================
// include/Camera.h
// ============================================================================
// ============================================================================
//  Camera.h - orientation, projection and view for the world's one viewpoint.
//
//  Camera used to fly itself around on WASD. It no longer does: Player.h now
//  owns position, velocity, gravity and ground/world collision for the thing
//  the person actually controls, and Application positions the camera each
//  frame relative to wherever the player ends up. In first person that's a
//  plain setPosition() to the player's eyes; in third person it's
//  followThirdPerson() below - an orbit camera that reuses the same
//  yaw/pitch mouse-look as first person (addLookDelta is untouched), just
//  anchored behind the player instead of driven directly.
// ============================================================================
#ifndef LIVINGISLAND_CAMERA_H
#define LIVINGISLAND_CAMERA_H

#include "Utilities.h"

class Terrain;

class Camera {
public:
    Camera();

    void setPosition(const util::Vec3& position);
    void lookAt(const util::Vec3& target);
    void addLookDelta(float dxPixels, float dyPixels);

    // Third person: the camera sits behind and above targetFeetPosition,
    // along the same direction the mouse is already looking (forward()),
    // eased into place rather than snapping - except when snap is true
    // (used the instant "V" is pressed, so flipping modes doesn't fly the
    // camera visibly across the scene). Held just above the terrain under
    // it so a slope behind the player can't put the camera underground.
    void followThirdPerson(float deltaTime, const util::Vec3& targetFeetPosition,
                            const Terrain& terrain, bool snap = false);

    void applyProjection(int windowWidth, int windowHeight) const;
    void applyView() const;

    util::Vec3 position() const { return m_position; }
    util::Vec3 forward()  const;
    util::Vec3 right()    const;
    util::Vec3 up()       const;
    float yaw()   const { return m_yaw; }
    float pitch() const { return m_pitch; }

    // Tunables (defaults pulled from Config.h)
    float sensitivity;
    float fieldOfView;
    float nearPlane;
    float farPlane;

private:
    util::Vec3 m_position;
    float m_yaw;     // degrees, 0 = +X
    float m_pitch;   // degrees, clamped to +/-89
};

#endif // LIVINGISLAND_CAMERA_H