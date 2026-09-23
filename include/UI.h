// ============================================================================
//  UI.h - screen-space HUD, control panel and draggable sliders.
//
//  Everything runs in an orthographic pass with lighting, fog and depth testing
//  switched off, restoring the 3D state afterwards.
//
//  The slider design deserves a note, because sections 4 and 40 pull in
//  opposite directions: pointer-locked mouse-look and draggable sliders cannot
//  both own the cursor. TAB releases the pointer and opens the panel; clicking
//  the view recaptures it. The sliders are only hit-testable while the pointer
//  is free, which removes the ambiguity entirely.
//
//  Values flow both ways. Normally the panel mirrors the world each frame; the
//  moment the player grabs a handle, the panel becomes authoritative for that
//  value until they let go.
// ============================================================================
#ifndef LIVINGISLAND_UI_H
#define LIVINGISLAND_UI_H

#include "GLIncludes.h"
#include "Utilities.h"

#include <string>

enum SliderId {
    SLIDER_TIME_OF_DAY = 0,
    SLIDER_TIME_SPEED,
    SLIDER_RAIN,
    SLIDER_WIND,
    SLIDER_CLOUD,
    SLIDER_COUNT
};

// The five controllable environment values, mirrored between UI and World.
struct ControlValues {
    float timeOfDay;      // hours, 0..24
    float timeSpeed;      // multiplier, 0..10
    float rainIntensity;  // 0..1
    float windStrength;   // 0..1
    float cloudDensity;   // 0..1

    ControlValues()
        : timeOfDay(10.5f), timeSpeed(1.0f), rainIntensity(0.0f)
        , windStrength(0.3f), cloudDensity(0.2f) {}
};

// Snapshot of everything the HUD displays. A plain struct keeps UI decoupled
// from World and Camera.
struct HudInfo {
    float       fps;
    util::Vec3  cameraPosition;
    float       cameraYaw;
    float       cameraPitch;
    std::string weatherName;

    int visibleTrees;
    int visibleGrass;
    int visibleProps;
    int visibleCharacters;
    int liveParticles;
    int particleCapacity;
    int characterCount;
    int treeCount;

    bool  mouseCaptured;
    bool  audioAvailable;
    bool  audioMuted;
    float startupFade;    // 1 at launch, 0 once the title has faded
    std::string interactionPrompt;  // empty when the player isn't near anything

    HudInfo()
        : fps(0.0f), cameraYaw(0.0f), cameraPitch(0.0f), weatherName("Clear")
        , visibleTrees(0), visibleGrass(0), visibleProps(0)
        , visibleCharacters(0), liveParticles(0), particleCapacity(0)
        , characterCount(0), treeCount(0)
        , mouseCaptured(true), audioAvailable(false), audioMuted(false)
        , startupFade(1.0f) {}
};

class UI {
public:
    UI();

    void setScreenSize(int width, int height);
    void render(const HudInfo& info);

    // --- control values ---
    void setControls(const ControlValues& values);
    const ControlValues& controls() const { return m_controls; }
    bool controlsChanged() const { return m_controlsChanged; }
    void clearControlsChanged() { m_controlsChanged = false; }

    // --- mouse interaction (only active while the pointer is free) ---
    bool handleMousePress(int x, int y);
    void handleMouseRelease();
    bool handleMouseDrag(int x, int y);
    bool isDragging() const { return m_draggingSlider >= 0; }

    bool hudVisible;
    bool panelVisible;
    bool debugVisible;
    bool helpVisible;

private:
    struct SliderLayout {
        float x, y, width, height;
        float minValue, maxValue;
        const char* label;
    };

    void beginOverlay() const;
    void endOverlay() const;

    void drawText(float x, float y, const std::string& text, void* font) const;
    void drawShadowedText(float x, float y, const std::string& text, void* font,
                          const util::Vec3& color, float alpha = 1.0f) const;
    void drawPanel(float x, float y, float w, float h,
                   float r, float g, float b, float a) const;
    void drawSlider(const SliderLayout& layout, float value, bool active) const;

    void layoutSliders();
    float valueFor(SliderId id) const;
    void  setValueFor(SliderId id, float value);

    void renderStatusPanel(const HudInfo& info) const;
    void renderControlPanel(const HudInfo& info) const;
    void renderDebugPanel(const HudInfo& info) const;
    void renderHelp(const HudInfo& info) const;
    void renderTitle(const HudInfo& info) const;
    void renderInteractionPrompt(const HudInfo& info) const;

    static std::string formatClock(float hours);

    int m_width;
    int m_height;

    ControlValues m_controls;
    bool m_controlsChanged;

    SliderLayout m_sliders[SLIDER_COUNT];
    int  m_draggingSlider;
};

#endif // LIVINGISLAND_UI_H
