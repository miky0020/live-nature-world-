#include "Input.h"

#include <cstring>
#include <cctype>

Input::Input()
    : m_shift(false)
    , m_leftDown(false)
    , m_mouseCaptured(true)
    , m_mouseX(0)
    , m_mouseY(0)
{
    std::memset(m_keys, 0, sizeof(m_keys));
    std::memset(m_specials, 0, sizeof(m_specials));
}

// GLUT reports 'W' while SHIFT is held and 'w' otherwise. If we stored them as
// different keys, releasing SHIFT mid-stride would leave a key stuck down.
unsigned char Input::normalize(unsigned char key) {
    return static_cast<unsigned char>(std::tolower(static_cast<int>(key)));
}

void Input::onKeyDown(unsigned char key) { m_keys[normalize(key)] = true; }
void Input::onKeyUp(unsigned char key)   { m_keys[normalize(key)] = false; }

void Input::onSpecialDown(int key) {
    if (key >= 0 && key < 256) m_specials[key] = true;
}

void Input::onSpecialUp(int key) {
    if (key >= 0 && key < 256) m_specials[key] = false;
}

void Input::setShift(bool held) { m_shift = held; }

void Input::setMousePosition(int x, int y) { m_mouseX = x; m_mouseY = y; }

void Input::setMouseButton(int button, bool pressed) {
    if (button == 0) m_leftDown = pressed;
}

void Input::releaseAll() {
    std::memset(m_keys, 0, sizeof(m_keys));
    std::memset(m_specials, 0, sizeof(m_specials));
    m_shift    = false;
    m_leftDown = false;
}

bool Input::key(unsigned char k) const { return m_keys[normalize(k)]; }

bool Input::special(int k) const {
    return (k >= 0 && k < 256) ? m_specials[k] : false;
}
