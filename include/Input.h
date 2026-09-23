// ============================================================================
//  Input.h - polled keyboard/mouse state.
//
//  GLUT only gives us events, but smooth movement needs "is W held right now".
//  This class turns the event stream into state the camera can poll each frame.
// ============================================================================
#ifndef LIVINGISLAND_INPUT_H
#define LIVINGISLAND_INPUT_H

class Input {
public:
    Input();

    // --- event feed (called from the GLUT callbacks) ---
    void onKeyDown(unsigned char key);
    void onKeyUp(unsigned char key);
    void onSpecialDown(int key);
    void onSpecialUp(int key);
    void setShift(bool held);
    void setMousePosition(int x, int y);
    void setMouseButton(int button, bool pressed);
    void releaseAll();                 // used when focus/capture changes

    // --- polled state ---
    bool key(unsigned char k) const;   // pass lowercase: key('w')
    bool special(int k) const;         // GLUT_KEY_*
    bool shift() const { return m_shift; }
    bool leftMouseDown() const { return m_leftDown; }
    int  mouseX() const { return m_mouseX; }
    int  mouseY() const { return m_mouseY; }

    // --- mouse capture (pointer locked for look, or free for the UI) ---
    void setMouseCaptured(bool captured) { m_mouseCaptured = captured; }
    bool mouseCaptured() const { return m_mouseCaptured; }

private:
    static unsigned char normalize(unsigned char key);

    bool m_keys[256];
    bool m_specials[256];
    bool m_shift;
    bool m_leftDown;
    bool m_mouseCaptured;
    int  m_mouseX;
    int  m_mouseY;
};

#endif // LIVINGISLAND_INPUT_H
