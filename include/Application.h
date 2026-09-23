// ============================================================================
// include/Application.h
// ============================================================================
// ============================================================================
//  Application.h - the one place GLUT's C-style callbacks touch.
//
//  GLUT callbacks are free functions with no user pointer, so *something* has
//  to be reachable statically. Rather than scattering globals across the
//  project there is exactly one static accessor here, and every callback
//  immediately forwards into a normal member function (section 34).
// ============================================================================
#ifndef LIVINGISLAND_APPLICATION_H
#define LIVINGISLAND_APPLICATION_H

#include "Camera.h"
#include "Fishing.h"
#include "Input.h"
#include "Player.h"
#include "UI.h"
#include "Utilities.h"
#include "World.h"

class Application {
public:
    static Application& instance();

    bool initialize(int argc, char** argv);
    void run();

private:
    Application();
    Application(const Application&);
    Application& operator=(const Application&);

    void setupOpenGLState();
    void update(float deltaTime);
    void render();
    void quit();

    void setMouseCaptured(bool captured);
    void refreshModifiers();
    void teleportPlayerTo(const util::Vec3& target, float distance, float height);
    void onInteractKey();
    HudInfo buildHudInfo() const;

    // --- instance-side handlers ---
    void onDisplay();
    void onReshape(int width, int height);
    void onKeyboard(unsigned char key, int x, int y);
    void onKeyboardUp(unsigned char key, int x, int y);
    void onSpecial(int key, int x, int y);
    void onSpecialUp(int key, int x, int y);
    void onMouseButton(int button, int state, int x, int y);
    void onMouseMotion(int x, int y);
    void onIdle();

    // --- static trampolines handed to GLUT ---
    static void displayCallback();
    static void reshapeCallback(int w, int h);
    static void keyboardCallback(unsigned char key, int x, int y);
    static void keyboardUpCallback(unsigned char key, int x, int y);
    static void specialCallback(int key, int x, int y);
    static void specialUpCallback(int key, int x, int y);
    static void mouseCallback(int button, int state, int x, int y);
    static void motionCallback(int x, int y);
    static void idleCallback();

    Camera      m_camera;
    Player      m_player;
    World       m_world;
    Input       m_input;
    UI          m_ui;
    FishingSystem m_fishing;
    util::Clock m_clock;

    int   m_windowWidth;
    int   m_windowHeight;
    float m_fps;
    float m_fpsAccumulator;
    int   m_fpsFrames;
    float m_startupTimer;
    float m_savedTimeSpeed;
    bool  m_initialized;
    bool  m_ignoreNextMotion;

    // Third-person follow is the next phase - the "V" key isn't bound yet.
    // Declared now so today's render()/avatar-visibility logic is already
    // correct and doesn't need touching again when V arrives. While this
    // stays false the player mesh isn't drawn: in first person the camera
    // sits at the avatar's own eye height, so the head/torso would otherwise
    // sit right in front of the lens.
    bool  m_thirdPerson;
    bool  m_ridingSwing;
};

#endif // LIVINGISLAND_APPLICATION_H