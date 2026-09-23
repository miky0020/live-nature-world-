// ============================================================================
// src/Application.cpp  (only the update() call site changed - added the
// collisionObstacles() argument; everything else is unchanged from Phase 3)
// ============================================================================
#include "Application.h"
#include "Config.h"
#include "GLIncludes.h"

#include <cstdlib>
#include <cstdio>
#include <string>

using util::Vec3;

// How long the title card lingers before fading out (section 32).
static const float STARTUP_TITLE_SECONDS = 4.0f;

// ---------------------------------------------------------------------------
Application::Application()
    : m_windowWidth(Config::WINDOW_WIDTH)
    , m_windowHeight(Config::WINDOW_HEIGHT)
    , m_fps(0.0f)
    , m_fpsAccumulator(0.0f)
    , m_fpsFrames(0)
    , m_startupTimer(0.0f)
    , m_savedTimeSpeed(1.0f)
    , m_initialized(false)
    , m_ignoreNextMotion(false)
    , m_thirdPerson(false)
    , m_ridingSwing(false)
{
}

Application& Application::instance() {
    static Application app;
    return app;
}

// ---------------------------------------------------------------------------
bool Application::initialize(int argc, char** argv) {
    util::logInfo("==================================================");
    util::logInfo("  LIVE NATURE - interactive 3D natural world");
    util::logInfo("==================================================");
    util::logInfo("Initializing OpenGL...");

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT);
    glutInitWindowPosition(0, 0);
    glutCreateWindow(Config::WINDOW_TITLE);

#ifndef __APPLE__
    // freeglut only: lets glutMainLoop() return so cleanup actually runs.
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
#endif

    // Without this, holding a key produces a down/up storm instead of a clean
    // "held" state, and movement stutters.
    glutIgnoreKeyRepeat(1);

    setupOpenGLState();

    m_world.initialize(Config::WORLD_SEED);

    // The player spawns standing on the ground at the same spot the free
    // camera used to open on, facing the village - the same "worth looking
    // at" framing section 31 always intended, just from head height instead
    // of hovering Config::CAM_START_Y units in the air, because there is now
    // someone actually standing there.
    float startX = Config::CAM_START_X;
    float startZ = Config::CAM_START_Z;
    float startGroundY = m_world.terrain().heightAt(startX, startZ);
    Vec3  startPosition(startX, startGroundY, startZ);

    Vec3 towardVillage = Vec3(0.0f, Config::VILLAGE_HEIGHT, 0.0f) - startPosition;
    float startFacing = util::toDegrees(std::atan2(towardVillage.x, towardVillage.z));

    m_player.initialize(startPosition, startFacing);
    m_camera.setPosition(m_player.eyePosition());
    m_camera.lookAt(Vec3(0.0f, Config::VILLAGE_HEIGHT + 2.0f, 0.0f));

    m_ui.setScreenSize(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT);
    m_ui.setControls(m_world.controls());

    glutDisplayFunc(displayCallback);
    glutReshapeFunc(reshapeCallback);
    glutKeyboardFunc(keyboardCallback);
    glutKeyboardUpFunc(keyboardUpCallback);
    glutSpecialFunc(specialCallback);
    glutSpecialUpFunc(specialUpCallback);
    glutMouseFunc(mouseCallback);
    glutMotionFunc(motionCallback);
    glutPassiveMotionFunc(motionCallback);
    glutIdleFunc(idleCallback);

    setMouseCaptured(true);

    m_clock.reset();
    m_initialized = true;

    util::logInfo("Ready. TAB frees the mouse for the sliders, F3 shows the controls.");
    return true;
}

void Application::setupOpenGLState() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glClearDepth(1.0);
    glClearColor(0.45f, 0.62f, 0.78f, 1.0f);

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif
    glEnable(GL_MULTISAMPLE);
}

void Application::run() {
    if (!m_initialized) {
        util::logError("Application::run() called before initialize().");
        return;
    }

    glutMainLoop();
    m_world.shutdown();
    util::logInfo("Shutdown complete.");
}

void Application::quit() {
    util::logInfo("Exit requested.");
    m_world.shutdown();

#ifndef __APPLE__
    glutLeaveMainLoop();
#else
    std::exit(0);
#endif
}

// ---------------------------------------------------------------------------
void Application::setMouseCaptured(bool captured) {
    m_input.setMouseCaptured(captured);

    if (captured) {
        glutSetCursor(GLUT_CURSOR_NONE);
        m_ignoreNextMotion = true;
        glutWarpPointer(m_windowWidth / 2, m_windowHeight / 2);
    } else {
        glutSetCursor(GLUT_CURSOR_LEFT_ARROW);

        // Stop movement keys sticking down while the player is in the UI.
        m_input.releaseAll();
        m_ui.handleMouseRelease();
    }
}

// GLUT has no event for modifier keys on their own, and glutGetModifiers() is
// only legal inside an input callback. Calling it from every input callback is
// the reliable workaround: while mouse-look is active, motion events arrive
// continuously, so SHIFT is picked up within a frame or two.
void Application::refreshModifiers() {
    int mods = glutGetModifiers();
    m_input.setShift((mods & GLUT_ACTIVE_SHIFT) != 0);
}

// Walk the player a comfortable distance from a landmark, facing it, and
// point the camera - which sits at the player's own eye position - up at
// whatever height of the landmark is worth looking at.
void Application::teleportPlayerTo(
    const Vec3& target,
    float distance,
    float lookHeight
) {
    if (m_player.isElevated())
        m_player.exitElevated();

    if (m_ridingSwing) {
        m_ridingSwing = false;
        m_player.endSwingRide(
            m_player.position(),
            m_player.facing()
        );
    }

    m_fishing.cancel();

    Vec3 offset(
        std::cos(2.2f) * distance,
        0.0f,
        std::sin(2.2f) * distance
    );

    Vec3 groundPosition =
        target + offset;

    groundPosition.y =
        m_world.terrain().heightAt(
            groundPosition.x,
            groundPosition.z
        );

    Vec3 towardTarget =
        target - groundPosition;

    float facing =
        util::toDegrees(
            std::atan2(
                towardTarget.x,
                towardTarget.z
            )
        );

    m_player.teleportTo(
        groundPosition,
        facing
    );

    m_camera.lookAt(
        target +
        Vec3(
            0.0f,
            lookHeight,
            0.0f
        )
    );

    if (m_thirdPerson) {
        m_camera.followThirdPerson(
            0.0f,
            m_player.position(),
            m_world.terrain(),
            /*snap=*/true
        );
    } else {
        m_camera.setPosition(
            m_player.eyePosition()
        );
    }
}

// ---------------------------------------------------------------------------
// 'E' - the one interaction key.
// ---------------------------------------------------------------------------
void Application::onInteractKey() {

    // ------------------------------------------------------------
    // Get off swing
    // ------------------------------------------------------------
    if (m_ridingSwing) {

        Vec3 base =
            m_world.swingPosition();

        Vec3 dismount =
            base;

        dismount.y =
            m_world.terrain().heightAt(
                base.x,
                base.z
            );

        m_player.endSwingRide(
            dismount,
            m_world.swingBaseFacing() + 180.0f
        );

        m_ridingSwing =
            false;

        if (m_thirdPerson) {

            m_camera.followThirdPerson(
                0.0f,
                m_player.position(),
                m_world.terrain(),
                true
            );

        } else {

            m_camera.setPosition(
                m_player.eyePosition()
            );
        }

        return;
    }


    // ------------------------------------------------------------
    // Elevated mode
    // ------------------------------------------------------------
    if (m_player.isElevated()) {
        m_player.exitElevated();
        return;
    }


    // ------------------------------------------------------------
    // Swing interaction
    // ------------------------------------------------------------
    if (m_world.nearSwing(
            m_player.position()
        ))
    {
        m_ridingSwing =
            true;

        m_player.beginSwingRide();


        if (m_thirdPerson) {

            m_camera.followThirdPerson(
                0.0f,
                m_world.swingSeatPosition(),
                m_world.terrain(),
                true
            );

        } else {

            Vec3 eye =
                m_world.swingSeatPosition()
                +
                Vec3(
                    0.0f,
                    Config::PLAYER_EYE_HEIGHT,
                    0.0f
                );

            m_camera.setPosition(
                eye
            );

            // Face the same direction as the riding player.
            // +180 makes the first-person view face the opposite
            // direction from the original Sakura facing.
            float yaw =
                util::toRadians(
                    m_world.swingBaseFacing() - 90.0f
                );

            m_camera.lookAt(
                eye +
                Vec3(
                    std::cos(yaw),
                    0.05f,
                    -std::sin(yaw)
                ) * 3.0f
            );
        }

        return;
    }


    // ------------------------------------------------------------
    // Generic interaction
    // ------------------------------------------------------------
    std::string label;

    if (!m_world.interactionPrompt(
            m_player.position(),
            label
        ))
    {
        return;
    }


    if (
        label.find("treehouse") !=
        std::string::npos
    )
    {
        m_player.enterElevated(
            m_world.treehousePosition(),
            m_world.treehousePlatformY(),
            Config::TREEHOUSE_PLATFORM_RADIUS
        );
    }
    else if (
        label.find("fish") !=
        std::string::npos
    )
    {
        m_fishing.onInteract();
    }

    // Campfire prompt is informational only.
}


// ---------------------------------------------------------------------------
void Application::update(
    float deltaTime
) {

    if (
        m_startupTimer <
        STARTUP_TITLE_SECONDS
    )
    {
        m_startupTimer +=
            deltaTime;
    }


    // ============================================================
    // SWING MODE
    // ============================================================

    if (m_ridingSwing) {

        // Riding fully bypasses normal player movement.
        float pump = 0.0f;

        if (m_input.key('w'))
            pump += 1.0f;

        if (m_input.key('s'))
            pump -= 1.0f;


        m_world.setSwingRider(
            true,
            pump
        );

        m_world.update(
            deltaTime,
            m_camera
        );


        // Player faces opposite the original swing base facing.
        m_player.followSwing(
            m_world.swingSeatPosition(),
            m_world.swingBaseFacing() - 90.0f,
            deltaTime
        );


        if (m_thirdPerson) {

            m_camera.followThirdPerson(
                deltaTime,
                m_player.position(),
                m_world.terrain()
            );

        } else {

            m_camera.setPosition(
                m_player.eyePosition()
            );
        }

    }


    // ============================================================
    // NORMAL PLAYER MODE
    // ============================================================

    else {

        m_world.setSwingRider(
            false,
            0.0f
        );


        m_player.update(
            deltaTime,
            m_input,
            m_camera.yaw(),
            m_world.terrain(),
            m_world.collisionObstacles()
        );


        if (m_thirdPerson) {

            m_camera.followThirdPerson(
                deltaTime,
                m_player.position(),
                m_world.terrain()
            );

        } else {

            m_camera.setPosition(
                m_player.eyePosition()
            );
        }


        m_world.update(
            deltaTime,
            m_camera
        );
    }


    // ============================================================
    // FISHING
    // ============================================================

    if (m_fishing.isActive()) {

        std::string label;

        bool nearLake =
            m_world.interactionPrompt(
                m_player.position(),
                label
            )
            &&
            label.find("fish") !=
                std::string::npos;


        if (!nearLake) {

            m_fishing.cancel();

        } else {

            m_fishing.update(
                deltaTime
            );
        }
    }


    // ============================================================
    // UI
    // ============================================================

    if (m_ui.controlsChanged()) {

        m_world.applyControls(
            m_ui.controls()
        );

        m_ui.clearControlsChanged();

    } else {

        m_ui.setControls(
            m_world.controls()
        );
    }


    // ============================================================
    // FPS
    // ============================================================

    m_fpsAccumulator +=
        deltaTime;

    ++m_fpsFrames;


    if (
        m_fpsAccumulator >=
        0.5f
    )
    {
        m_fps =
            static_cast<float>(
                m_fpsFrames
            ) /
            m_fpsAccumulator;

        m_fpsAccumulator =
            0.0f;

        m_fpsFrames =
            0;
    }
}


// ---------------------------------------------------------------------------
HudInfo Application::buildHudInfo() const {

    const RenderContext& context =
        m_world.lastContext();


    HudInfo info;


    info.fps =
        m_fps;

    info.cameraPosition =
        m_camera.position();

    info.cameraYaw =
        m_camera.yaw();

    info.cameraPitch =
        m_camera.pitch();

    info.weatherName =
        m_world.weatherName();


    info.visibleTrees =
        context.visibleTrees;

    info.visibleGrass =
        context.visibleGrass;

    info.visibleProps =
        context.visibleProps;

    info.visibleCharacters =
        context.visibleCharacters;

    info.liveParticles =
        context.liveParticles;

    info.particleCapacity =
        m_world.particleCapacity();

    info.characterCount =
        m_world.characterCount();

    info.treeCount =
        m_world.treeCount();


    info.mouseCaptured =
        m_input.mouseCaptured();

    info.audioAvailable =
        const_cast<World&>(
            m_world
        ).audio().available();

    info.audioMuted =
        const_cast<World&>(
            m_world
        ).audio().muted();


    float remaining =
        STARTUP_TITLE_SECONDS -
        m_startupTimer;

    info.startupFade =
        util::clampf(
            remaining /
            STARTUP_TITLE_SECONDS,
            0.0f,
            1.0f
        );


    if (m_ridingSwing) {

        info.interactionPrompt =
            "Press E to hop off (W/S to pump)";

    } else if (
        m_world.nearSwing(
            m_player.position()
        )
    ) {

        info.interactionPrompt =
            "Press E to use Swing";

    } else if (
        m_fishing.isActive()
    ) {

        info.interactionPrompt =
            m_fishing.statusText();

    } else if (
        m_player.isElevated()
    ) {

        info.interactionPrompt =
            "Press E to climb down";
    }


    return info;
}


// ---------------------------------------------------------------------------
void Application::render() {

    m_camera.applyProjection(
        m_windowWidth,
        m_windowHeight
    );

    m_world.render(
        m_camera
    );


    // First person:
    // camera is already at player's eyes.
    //
    // Third person:
    // render the player avatar.
    if (m_thirdPerson)
        m_player.render(
            m_world.lastContext()
        );


    m_ui.render(
        buildHudInfo()
    );


    glutSwapBuffers();
}


// ---------------------------------------------------------------------------
void Application::onDisplay() {
    render();
}


// ---------------------------------------------------------------------------
void Application::onReshape(
    int width,
    int height
) {

    if (height <= 0)
        height = 1;

    if (width <= 0)
        width = 1;


    m_windowWidth =
        width;

    m_windowHeight =
        height;


    glViewport(
        0,
        0,
        width,
        height
    );


    m_camera.applyProjection(
        width,
        height
    );


    m_ui.setScreenSize(
        width,
        height
    );
}


// ---------------------------------------------------------------------------
void Application::onKeyboard(
    unsigned char key,
    int,
    int
) {

    refreshModifiers();


    switch (key) {

        // --------------------------------------------------------
        // ESC
        // --------------------------------------------------------
        case 27:
            quit();
            return;


        // --------------------------------------------------------
        // TAB
        // --------------------------------------------------------
        case 9:
            setMouseCaptured(
                !m_input.mouseCaptured()
            );
            return;


        // --------------------------------------------------------
        // Weather
        // --------------------------------------------------------
        case '1':
            m_world.setWeatherType(
                WEATHER_CLEAR
            );
            return;

        case '2':
            m_world.setWeatherType(
                WEATHER_RAIN
            );
            return;

        case '3':
            m_world.setWeatherType(
                WEATHER_HEAVY_RAIN
            );
            return;


        // --------------------------------------------------------
        // Time
        // --------------------------------------------------------
        case '4':
            m_world.setTimeOfDay(
                12.0f
            );
            return;

        case '5':
            m_world.setTimeOfDay(
                0.5f
            );
            return;


        // --------------------------------------------------------
        // Demo shortcuts
        // --------------------------------------------------------
        case '6':
            teleportPlayerTo(
                m_world.campfirePosition(),
                9.0f,
                2.6f
            );
            return;

        case '7':
            teleportPlayerTo(
                m_world.treehousePosition(),
                15.0f,
                5.5f
            );
            return;

        case '8':
            m_world.triggerEmote();
            return;

        case '9':
            m_world.setWeatherType(
                WEATHER_CLOUDY
            );
            return;


        // --------------------------------------------------------
        // Time speed
        // --------------------------------------------------------
        case '[':
            m_world.setTimeSpeed(
                m_world.timeSpeed() - 1.0f
            );
            return;

        case ']':
            m_world.setTimeSpeed(
                m_world.timeSpeed() + 1.0f
            );
            return;


        // --------------------------------------------------------
        // Pause / resume
        // --------------------------------------------------------
        case 'p':
        case 'P': {

            if (
                m_world.timeSpeed() >
                0.0f
            ) {

                m_savedTimeSpeed =
                    m_world.timeSpeed();

                m_world.setTimeSpeed(
                    0.0f
                );

                util::logInfo(
                    "Time paused."
                );

            } else {

                m_world.setTimeSpeed(
                    m_savedTimeSpeed >
                    0.0f
                        ? m_savedTimeSpeed
                        : 1.0f
                );

                util::logInfo(
                    "Time resumed."
                );
            }

            return;
        }


        // --------------------------------------------------------
        // Audio
        // --------------------------------------------------------
        case 'm':
        case 'M':

            m_world.audio()
                .toggleMute();

            return;


        // --------------------------------------------------------
        // Ground collision debug
        // --------------------------------------------------------
        case 'c':
        case 'C':

            m_player.collideWithGround =
                !m_player.collideWithGround;

            util::logInfo(
                m_player.collideWithGround
                    ? "Player ground collision ON."
                    : "Player ground collision OFF (debug)."
            );

            return;


        // --------------------------------------------------------
        // Third person
        // --------------------------------------------------------
        case 'v':
        case 'V':

            m_thirdPerson =
                !m_thirdPerson;


            if (m_thirdPerson) {

                m_camera.followThirdPerson(
                    0.0f,
                    m_player.position(),
                    m_world.terrain(),
                    /*snap=*/true
                );

            } else {

                m_camera.setPosition(
                    m_player.eyePosition()
                );
            }


            util::logInfo(
                m_thirdPerson
                    ? "Third-person camera."
                    : "First-person camera."
            );

            return;


        // --------------------------------------------------------
        // E - interaction
        // --------------------------------------------------------
        case 'e':
        case 'E':

            onInteractKey();

            return;


        default:
            break;
    }


    m_input.onKeyDown(
        key
    );
}


// ---------------------------------------------------------------------------
void Application::onKeyboardUp(
    unsigned char key,
    int,
    int
) {

    refreshModifiers();

    m_input.onKeyUp(
        key
    );
}


// ---------------------------------------------------------------------------
void Application::onSpecial(
    int key,
    int,
    int
) {

    refreshModifiers();


    switch (key) {

        case GLUT_KEY_F1:
            m_ui.debugVisible =
                !m_ui.debugVisible;
            return;

        case GLUT_KEY_F2:
            m_ui.hudVisible =
                !m_ui.hudVisible;
            return;

        case GLUT_KEY_F3:
            m_ui.helpVisible =
                !m_ui.helpVisible;
            return;

        default:
            break;
    }


    m_input.onSpecialDown(
        key
    );
}


// ---------------------------------------------------------------------------
void Application::onSpecialUp(
    int key,
    int,
    int
) {

    refreshModifiers();

    m_input.onSpecialUp(
        key
    );
}


// ---------------------------------------------------------------------------
void Application::onMouseButton(
    int button,
    int state,
    int x,
    int y
) {

    refreshModifiers();

    m_input.setMousePosition(
        x,
        y
    );


    if (
        button !=
        GLUT_LEFT_BUTTON
    )
        return;


    m_input.setMouseButton(
        0,
        state == GLUT_DOWN
    );


    if (
        state ==
        GLUT_DOWN
    ) {

        if (
            !m_input.mouseCaptured()
        ) {

            // A click on a slider goes to the UI;
            // a click anywhere else means
            // "give me the camera back".
            if (
                m_ui.handleMousePress(
                    x,
                    y
                )
            )
                return;


            setMouseCaptured(
                true
            );
        }

    } else {

        m_ui.handleMouseRelease();
    }
}


// ---------------------------------------------------------------------------
void Application::onMouseMotion(
    int x,
    int y
) {

    refreshModifiers();

    m_input.setMousePosition(
        x,
        y
    );


    if (
        !m_input.mouseCaptured()
    ) {

        m_ui.handleMouseDrag(
            x,
            y
        );

        return;
    }


    const int centreX =
        m_windowWidth / 2;

    const int centreY =
        m_windowHeight / 2;


    // The warp below generates its own motion event;
    // swallow it or the camera would fight itself.
    if (
        m_ignoreNextMotion ||
        (x == centreX && y == centreY)
    ) {

        m_ignoreNextMotion =
            false;

        return;
    }


    m_camera.addLookDelta(
        static_cast<float>(
            x - centreX
        ),
        static_cast<float>(
            y - centreY
        )
    );


    m_ignoreNextMotion =
        true;

    glutWarpPointer(
        centreX,
        centreY
    );
}


// ---------------------------------------------------------------------------
void Application::onIdle() {

    float deltaTime =
        m_clock.tick();

    update(
        deltaTime
    );

    glutPostRedisplay();
}


// --------------------------------------------------- static trampolines -----

void Application::displayCallback()
{
    instance().onDisplay();
}

void Application::reshapeCallback(
    int w,
    int h
)
{
    instance().onReshape(
        w,
        h
    );
}

void Application::keyboardCallback(
    unsigned char k,
    int x,
    int y
)
{
    instance().onKeyboard(
        k,
        x,
        y
    );
}

void Application::keyboardUpCallback(
    unsigned char k,
    int x,
    int y
)
{
    instance().onKeyboardUp(
        k,
        x,
        y
    );
}

void Application::specialCallback(
    int k,
    int x,
    int y
)
{
    instance().onSpecial(
        k,
        x,
        y
    );
}

void Application::specialUpCallback(
    int k,
    int x,
    int y
)
{
    instance().onSpecialUp(
        k,
        x,
        y
    );
}

void Application::mouseCallback(
    int b,
    int s,
    int x,
    int y
)
{
    instance().onMouseButton(
        b,
        s,
        x,
        y
    );
}

void Application::motionCallback(
    int x,
    int y
)
{
    instance().onMouseMotion(
        x,
        y
    );
}

void Application::idleCallback()
{
    instance().onIdle();
}