// ============================================================================
// src/UI.cpp
// ============================================================================
#include "UI.h"
#include "Config.h"

#include <cstdio>

using util::Vec3;

UI::UI()
    : hudVisible(true)
    , panelVisible(true)
    , debugVisible(false)
    , helpVisible(false)
    , m_width(Config::WINDOW_WIDTH)
    , m_height(Config::WINDOW_HEIGHT)
    , m_controlsChanged(false)
    , m_draggingSlider(-1)
{
    layoutSliders();
}

void UI::setScreenSize(int width, int height) {
    m_width  = width  > 0 ? width  : 1;
    m_height = height > 0 ? height : 1;
    layoutSliders();
}

void UI::layoutSliders() {
    const float panelX = 16.0f;
    const float panelW = 250.0f;
    const float sliderH = 12.0f;
    const float spacing = 34.0f;

    // Anchored to the bottom-left so the panel never fights the status readout
    // and never covers the middle of the view.
    float firstY = static_cast<float>(m_height) - 40.0f - spacing * (SLIDER_COUNT - 1);

    const char* labels[SLIDER_COUNT] = {
        "TIME OF DAY", "TIME SPEED", "RAIN INTENSITY", "WIND", "CLOUD DENSITY"
    };
    const float minimums[SLIDER_COUNT] = { 0.0f, 0.0f,  0.0f, 0.0f, 0.0f };
    const float maximums[SLIDER_COUNT] = { 24.0f, 10.0f, 1.0f, 1.0f, 1.0f };

    for (int i = 0; i < SLIDER_COUNT; ++i) {
        m_sliders[i].x = panelX + 14.0f;
        m_sliders[i].y = firstY + static_cast<float>(i) * spacing;
        m_sliders[i].width  = panelW - 28.0f;
        m_sliders[i].height = sliderH;
        m_sliders[i].minValue = minimums[i];
        m_sliders[i].maxValue = maximums[i];
        m_sliders[i].label = labels[i];
    }
}

// ---------------------------------------------------------------------------
void UI::setControls(const ControlValues& values) {
    // A live drag owns its value: overwriting it from the world would make the
    // handle jump out from under the pointer.
    if (m_draggingSlider >= 0) {
        float held = valueFor(static_cast<SliderId>(m_draggingSlider));
        m_controls = values;
        setValueFor(static_cast<SliderId>(m_draggingSlider), held);
    } else {
        m_controls = values;
    }
}

float UI::valueFor(SliderId id) const {
    switch (id) {
        case SLIDER_TIME_OF_DAY: return m_controls.timeOfDay;
        case SLIDER_TIME_SPEED:  return m_controls.timeSpeed;
        case SLIDER_RAIN:        return m_controls.rainIntensity;
        case SLIDER_WIND:        return m_controls.windStrength;
        case SLIDER_CLOUD:       return m_controls.cloudDensity;
        default: return 0.0f;
    }
}

void UI::setValueFor(SliderId id, float value) {
    switch (id) {
        case SLIDER_TIME_OF_DAY: m_controls.timeOfDay     = value; break;
        case SLIDER_TIME_SPEED:  m_controls.timeSpeed     = value; break;
        case SLIDER_RAIN:        m_controls.rainIntensity = value; break;
        case SLIDER_WIND:        m_controls.windStrength  = value; break;
        case SLIDER_CLOUD:       m_controls.cloudDensity  = value; break;
        default: break;
    }
}

bool UI::handleMousePress(int x, int y) {
    if (!panelVisible || !hudVisible) return false;

    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);

    for (int i = 0; i < SLIDER_COUNT; ++i) {
        const SliderLayout& s = m_sliders[i];
        // Generous vertical margin: a 12px track is hard to hit precisely.
        if (fx >= s.x - 6.0f && fx <= s.x + s.width + 6.0f
            && fy >= s.y - 8.0f && fy <= s.y + s.height + 8.0f) {
            m_draggingSlider = i;
            handleMouseDrag(x, y);
            return true;
        }
    }
    return false;
}

void UI::handleMouseRelease() { m_draggingSlider = -1; }

bool UI::handleMouseDrag(int x, int y) {
    if (m_draggingSlider < 0) return false;
    (void)y;

    const SliderLayout& s = m_sliders[m_draggingSlider];
    float t = util::clampf((static_cast<float>(x) - s.x) / s.width, 0.0f, 1.0f);

    setValueFor(static_cast<SliderId>(m_draggingSlider),
                util::lerpf(s.minValue, s.maxValue, t));
    m_controlsChanged = true;
    return true;
}

// ---------------------------------------------------------------------------
//  Overlay plumbing. Origin is top-left, y grows downward.
// ---------------------------------------------------------------------------
void UI::beginOverlay() const {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, static_cast<GLdouble>(m_width),
               static_cast<GLdouble>(m_height), 0.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT
                | GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void UI::endOverlay() const {
    glPopAttrib();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void UI::drawText(float x, float y, const std::string& text, void* font) const {
    glRasterPos2f(x, y);
    for (size_t i = 0; i < text.size(); ++i) {
        glutBitmapCharacter(font, static_cast<int>(static_cast<unsigned char>(text[i])));
    }
}

void UI::drawShadowedText(float x, float y, const std::string& text, void* font,
                          const Vec3& color, float alpha) const
{
    glColor4f(0.0f, 0.0f, 0.0f, 0.62f * alpha);
    drawText(x + 1.0f, y + 1.0f, text, font);
    glColor4f(color.x, color.y, color.z, alpha);
    drawText(x, y, text, font);
}

void UI::drawPanel(float x, float y, float w, float h,
                   float r, float g, float b, float a) const
{
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
        glVertex2f(x,     y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x,     y + h);
    glEnd();

    glColor4f(1.0f, 1.0f, 1.0f, a * 0.25f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x + 0.5f,     y + 0.5f);
        glVertex2f(x + w - 0.5f, y + 0.5f);
        glVertex2f(x + w - 0.5f, y + h - 0.5f);
        glVertex2f(x + 0.5f,     y + h - 0.5f);
    glEnd();
}

// A filled-bar slider with a handle, matching the mock-up in section 40.
void UI::drawSlider(const SliderLayout& layout, float value, bool active) const {
    float range = layout.maxValue - layout.minValue;
    float t = range > 1e-6f ? util::clampf((value - layout.minValue) / range, 0.0f, 1.0f) : 0.0f;

    // Label
    drawShadowedText(layout.x, layout.y - 5.0f, layout.label,
                     GLUT_BITMAP_HELVETICA_10, Vec3(0.78f, 0.84f, 0.92f));

    // Numeric readout, right-aligned-ish
    char readout[32];
    if (layout.maxValue > 20.0f) {
        std::snprintf(readout, sizeof(readout), "%s", formatClock(value).c_str());
    } else if (layout.maxValue > 5.0f) {
        std::snprintf(readout, sizeof(readout), "%.1fx", value);
    } else {
        std::snprintf(readout, sizeof(readout), "%.0f%%", value * 100.0f);
    }
    drawShadowedText(layout.x + layout.width - 38.0f, layout.y - 5.0f, readout,
                     GLUT_BITMAP_HELVETICA_10, Vec3(0.95f, 0.97f, 1.0f));

    // Track
    glColor4f(0.10f, 0.13f, 0.17f, 0.85f);
    glBegin(GL_QUADS);
        glVertex2f(layout.x,                layout.y);
        glVertex2f(layout.x + layout.width, layout.y);
        glVertex2f(layout.x + layout.width, layout.y + layout.height);
        glVertex2f(layout.x,                layout.y + layout.height);
    glEnd();

    // Fill
    float fillWidth = layout.width * t;
    Vec3 fill = active ? Vec3(0.55f, 0.85f, 0.65f) : Vec3(0.38f, 0.66f, 0.52f);
    glColor4f(fill.x, fill.y, fill.z, 0.92f);
    glBegin(GL_QUADS);
        glVertex2f(layout.x,             layout.y + 1.0f);
        glVertex2f(layout.x + fillWidth, layout.y + 1.0f);
        glVertex2f(layout.x + fillWidth, layout.y + layout.height - 1.0f);
        glVertex2f(layout.x,             layout.y + layout.height - 1.0f);
    glEnd();

    // Handle
    float handleX = layout.x + fillWidth;
    glColor4f(0.96f, 0.98f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
        glVertex2f(handleX - 3.0f, layout.y - 3.0f);
        glVertex2f(handleX + 3.0f, layout.y - 3.0f);
        glVertex2f(handleX + 3.0f, layout.y + layout.height + 3.0f);
        glVertex2f(handleX - 3.0f, layout.y + layout.height + 3.0f);
    glEnd();

    // Border
    glColor4f(1.0f, 1.0f, 1.0f, 0.30f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(layout.x + 0.5f,                layout.y + 0.5f);
        glVertex2f(layout.x + layout.width - 0.5f, layout.y + 0.5f);
        glVertex2f(layout.x + layout.width - 0.5f, layout.y + layout.height - 0.5f);
        glVertex2f(layout.x + 0.5f,                layout.y + layout.height - 0.5f);
    glEnd();
}

std::string UI::formatClock(float hours) {
    float wrapped = util::wrapf(hours, 24.0f);
    int h = static_cast<int>(wrapped);
    int m = static_cast<int>((wrapped - static_cast<float>(h)) * 60.0f);
    if (m > 59) m = 59;

    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d", h, m);
    return std::string(buffer);
}

// ---------------------------------------------------------------------------
//  Panels
// ---------------------------------------------------------------------------
void UI::renderStatusPanel(const HudInfo& info) const {
    const Vec3 textColor(0.94f, 0.96f, 1.00f);
    const Vec3 accent(0.72f, 0.90f, 0.66f);
    char line[192];

    drawPanel(16.0f, 16.0f, 250.0f, 98.0f, 0.04f, 0.07f, 0.10f, 0.48f);

    drawShadowedText(28.0f, 38.0f, "LIVE NATURE", GLUT_BITMAP_HELVETICA_12, accent);

    std::snprintf(line, sizeof(line), "TIME  %s        FPS  %.0f",
                  formatClock(m_controls.timeOfDay).c_str(), info.fps);
    drawShadowedText(28.0f, 60.0f, line, GLUT_BITMAP_HELVETICA_12, textColor);

    std::snprintf(line, sizeof(line), "WEATHER  %s", info.weatherName.c_str());
    drawShadowedText(28.0f, 78.0f, line, GLUT_BITMAP_HELVETICA_12, textColor);

    std::snprintf(line, sizeof(line), "WIND %.0f%%   RAIN %.0f%%   CLOUD %.0f%%",
                  m_controls.windStrength * 100.0f,
                  m_controls.rainIntensity * 100.0f,
                  m_controls.cloudDensity * 100.0f);
    drawShadowedText(28.0f, 96.0f, line, GLUT_BITMAP_HELVETICA_12, textColor);

    std::snprintf(line, sizeof(line), "AUDIO  %s",
                  !info.audioAvailable ? "unavailable"
                                       : (info.audioMuted ? "muted" : "on"));
    drawShadowedText(28.0f, 110.0f, line, GLUT_BITMAP_HELVETICA_10,
                     Vec3(0.70f, 0.76f, 0.84f));
}

void UI::renderControlPanel(const HudInfo& info) const {
    float panelTop = m_sliders[0].y - 26.0f;
    float panelBottom = m_sliders[SLIDER_COUNT - 1].y + m_sliders[SLIDER_COUNT - 1].height + 14.0f;

    drawPanel(16.0f, panelTop - 22.0f, 250.0f,
              (panelBottom - panelTop) + 22.0f,
              0.04f, 0.07f, 0.10f, 0.48f);

    drawShadowedText(28.0f, panelTop - 6.0f,
                     info.mouseCaptured ? "CONTROLS  (TAB to adjust)" : "CONTROLS  (drag me)",
                     GLUT_BITMAP_HELVETICA_10,
                     info.mouseCaptured ? Vec3(0.60f, 0.66f, 0.74f)
                                        : Vec3(0.72f, 0.90f, 0.66f));

    for (int i = 0; i < SLIDER_COUNT; ++i) {
        drawSlider(m_sliders[i], valueFor(static_cast<SliderId>(i)),
                   !info.mouseCaptured && m_draggingSlider == i);
    }
}

void UI::renderDebugPanel(const HudInfo& info) const {
    const Vec3 textColor(0.92f, 0.95f, 1.00f);
    const Vec3 accent(0.72f, 0.90f, 0.66f);
    char line[160];

    float x = static_cast<float>(m_width) - 246.0f;
    drawPanel(x, 16.0f, 230.0f, 158.0f, 0.04f, 0.07f, 0.10f, 0.48f);

    drawShadowedText(x + 12.0f, 38.0f, "DEBUG  [F1]", GLUT_BITMAP_HELVETICA_12, accent);

    std::snprintf(line, sizeof(line), "POS   %.1f, %.1f, %.1f",
                  info.cameraPosition.x, info.cameraPosition.y, info.cameraPosition.z);
    drawShadowedText(x + 12.0f, 58.0f, line, GLUT_BITMAP_HELVETICA_10, textColor);

    std::snprintf(line, sizeof(line), "ROT   yaw %.1f   pitch %.1f",
                  info.cameraYaw, info.cameraPitch);
    drawShadowedText(x + 12.0f, 74.0f, line, GLUT_BITMAP_HELVETICA_10, textColor);

    std::snprintf(line, sizeof(line), "TIME  %s  at %.1fx",
                  formatClock(m_controls.timeOfDay).c_str(), m_controls.timeSpeed);
    drawShadowedText(x + 12.0f, 90.0f, line, GLUT_BITMAP_HELVETICA_10, textColor);

    std::snprintf(line, sizeof(line), "TREES  %d / %d visible",
                  info.visibleTrees, info.treeCount);
    drawShadowedText(x + 12.0f, 110.0f, line, GLUT_BITMAP_HELVETICA_10, textColor);

    std::snprintf(line, sizeof(line), "GRASS  %d visible", info.visibleGrass);
    drawShadowedText(x + 12.0f, 126.0f, line, GLUT_BITMAP_HELVETICA_10, textColor);

    std::snprintf(line, sizeof(line), "PROPS  %d    PEOPLE  %d / %d",
                  info.visibleProps, info.visibleCharacters, info.characterCount);
    drawShadowedText(x + 12.0f, 142.0f, line, GLUT_BITMAP_HELVETICA_10, textColor);

    std::snprintf(line, sizeof(line), "PARTICLES  %d / %d",
                  info.liveParticles, info.particleCapacity);
    drawShadowedText(x + 12.0f, 158.0f, line, GLUT_BITMAP_HELVETICA_10, textColor);

    std::snprintf(line, sizeof(line), "VIEWPORT  %d x %d", m_width, m_height);
    drawShadowedText(x + 12.0f, 172.0f, line, GLUT_BITMAP_HELVETICA_10,
                     Vec3(0.66f, 0.72f, 0.80f));
}

void UI::renderHelp(const HudInfo& info) const {
    (void)info;

    const char* lines[] = {
        "MOVEMENT",
        "  W / A / S / D     move",
        "  SHIFT             run",
        "  SPACE             jump",
        "  MOUSE             look around",
        "  V                 first / third person",
        "",
        "INTERACT (walk up to the prompt, then press E)",
        "  E                 climb the treehouse",
        "  E                 sit on the swing (W/S to pump)",
        "  E                 fish at the lake (E again on the bite)",
        "",
        "WORLD",
        "  1                 clear weather",
        "  2                 rain",
        "  3                 heavy rain",
        "  4                 jump to midday",
        "  5                 jump to night",
        "  6                 go to the campfire",
        "  7                 go to the treehouse",
        "  8                 trigger an emote",
        "  [ / ]             time speed down / up",
        "  P                 pause time",
        "",
        "DEBUG",
        "  C                 toggle player ground collision",
        "",
        "INTERFACE",
        "  TAB               release mouse / use sliders",
        "  F1                debug info",
        "  F2                hide the HUD",
        "  F3                this help",
        "  M                 mute audio",
        "  ESC               quit"
    };
    const int lineCount = static_cast<int>(sizeof(lines) / sizeof(lines[0]));

    float panelW = 300.0f;
    float panelH = static_cast<float>(lineCount) * 15.0f + 34.0f;
    float x = (static_cast<float>(m_width) - panelW) * 0.5f;
    float y = (static_cast<float>(m_height) - panelH) * 0.5f;

    drawPanel(x, y, panelW, panelH, 0.03f, 0.05f, 0.08f, 0.80f);

    drawShadowedText(x + 18.0f, y + 24.0f, "CONTROLS   [F3 to close]",
                     GLUT_BITMAP_HELVETICA_12, Vec3(0.72f, 0.90f, 0.66f));

    for (int i = 0; i < lineCount; ++i) {
        drawShadowedText(x + 18.0f, y + 44.0f + static_cast<float>(i) * 15.0f,
                         lines[i], GLUT_BITMAP_HELVETICA_10,
                         Vec3(0.88f, 0.92f, 0.98f));
    }
}

// Section 32: show the title, then get out of the way.
void UI::renderTitle(const HudInfo& info) const {
    if (info.startupFade <= 0.01f) return;

    float alpha = util::smoothstepf(0.0f, 0.35f, info.startupFade);

    float centreX = static_cast<float>(m_width) * 0.5f;
    float centreY = static_cast<float>(m_height) * 0.42f;

    const char* title = "LIVE NATURE";
    int titleWidth = glutBitmapLength(GLUT_BITMAP_TIMES_ROMAN_24,
                                      reinterpret_cast<const unsigned char*>(title));

    drawShadowedText(centreX - static_cast<float>(titleWidth) * 0.5f, centreY,
                     title, GLUT_BITMAP_TIMES_ROMAN_24,
                     Vec3(1.0f, 1.0f, 1.0f), alpha);

    const char* subtitle = "press F3 for controls";
    int subWidth = glutBitmapLength(GLUT_BITMAP_HELVETICA_12,
                                    reinterpret_cast<const unsigned char*>(subtitle));
    drawShadowedText(centreX - static_cast<float>(subWidth) * 0.5f, centreY + 26.0f,
                     subtitle, GLUT_BITMAP_HELVETICA_12,
                     Vec3(0.86f, 0.92f, 1.0f), alpha * 0.9f);
}

// ---------------------------------------------------------------------------
void UI::render(const HudInfo& info) {
    beginOverlay();

    if (hudVisible) {
        renderStatusPanel(info);
        if (panelVisible) renderControlPanel(info);

        const char* hint = info.mouseCaptured
            ? "WASD move   SHIFT run   SPACE jump   V camera   TAB release mouse   F3 help   ESC quit"
            : "MOUSE FREE - drag the sliders, or click the view to look around again";
        drawShadowedText(18.0f, static_cast<float>(m_height) - 14.0f, hint,
                         GLUT_BITMAP_HELVETICA_10, Vec3(0.80f, 0.85f, 0.92f));
    }

    if (debugVisible) renderDebugPanel(info);
    if (helpVisible)  renderHelp(info);
    renderTitle(info);
    renderInteractionPrompt(info);

    endOverlay();
}

void UI::renderInteractionPrompt(const HudInfo& info) const {
    if (info.interactionPrompt.empty()) return;

    float centreX = static_cast<float>(m_width) * 0.5f;
    float y = static_cast<float>(m_height) - 46.0f;

    int textWidth = glutBitmapLength(GLUT_BITMAP_HELVETICA_18,
        reinterpret_cast<const unsigned char*>(info.interactionPrompt.c_str()));

    drawPanel(centreX - static_cast<float>(textWidth) * 0.5f - 14.0f, y - 20.0f,
              static_cast<float>(textWidth) + 28.0f, 30.0f,
              0.04f, 0.07f, 0.10f, 0.55f);
    drawShadowedText(centreX - static_cast<float>(textWidth) * 0.5f, y,
                     info.interactionPrompt, GLUT_BITMAP_HELVETICA_18,
                     Vec3(0.95f, 0.92f, 0.70f));
}