#include "input.h"
#include <avr/pgmspace.h>

void JoystickReader::calibrate() {
  long sx = 0, sy = 0;
  for (uint8_t i = 0; i < 16; i++) {
    sx += analogRead(PIN_JOY_X);
    sy += analogRead(PIN_JOY_Y);
    delay(5);
  }
  _centerX = sx / 16;
  _centerY = sy / 16;
}

JoyDir JoystickReader::update() {
  int x = analogRead(PIN_JOY_X);
  int y = analogRead(PIN_JOY_Y);
  int dx = x - _centerX;
  int dy = y - _centerY;

  int lr = JOY_SWAP_XY ? dy : dx;
  int ud = JOY_SWAP_XY ? dx : dy;
  if (JOY_INVERT_LR) lr = -lr;
  if (JOY_INVERT_UD) ud = -ud;

  if (!_armed) {
    if (abs(lr) < JOY_DEADZONE && abs(ud) < JOY_DEADZONE) {
      _armed = true;
    }
    return JoyDir::NONE;
  }

  if (abs(ud) >= abs(lr)) {
    if (ud > JOY_PUSH_THRESHOLD) { _armed = false; return JoyDir::UP; }
    if (ud < -JOY_PUSH_THRESHOLD) { _armed = false; return JoyDir::DOWN; }
  } else {
    if (lr > JOY_PUSH_THRESHOLD) { _armed = false; return JoyDir::RIGHT; }
    if (lr < -JOY_PUSH_THRESHOLD) { _armed = false; return JoyDir::LEFT; }
  }
  return JoyDir::NONE;
}

// ---- Mode display names (PROGMEM), indexed directly by ModeId ----
static const char NAME_PERF[] PROGMEM = "Performance";
static const char NAME_VIS[] PROGMEM = "Visualizer";
static const char NAME_DINO[] PROGMEM = "Dino";
static const char NAME_PET[] PROGMEM = "Desk Pet";
static const char NAME_HACKING[] PROGMEM = "Hacking";
static const char NAME_WHEEL[] PROGMEM = "Wheel";
static const char NAME_COFFEE[] PROGMEM = "Coffee";

static const char* const MODE_NAMES[] PROGMEM = {
  NAME_PERF, NAME_VIS, NAME_DINO, NAME_PET, NAME_HACKING, NAME_WHEEL, NAME_COFFEE
};

void copyModeName(uint8_t modeId, char* out, uint8_t outLen) {
  const char* p = (const char*)pgm_read_word(&MODE_NAMES[modeId]);
  strncpy_P(out, p, outLen - 1);
  out[outLen - 1] = '\0';
}

void MenuSystem::begin() {
  _state = State::CLOSED;
}

bool MenuSystem::consumeLaunchEvent() {
  if (_launchPending) {
    _launchPending = false;
    return true;
  }
  return false;
}

bool MenuSystem::consumeClickEvent() {
  if (_clickPending) {
    _clickPending = false;
    return true;
  }
  return false;
}

void MenuSystem::commitAction(State fromState, unsigned long now) {
  if (fromState == State::CLOSED) {
    _state = State::OPEN;
  } else { // OPEN -> confirm the highlighted selection
    _lastModeId = _selectedIndex;
    _launchPending = true;
    _state = State::CLOSED;
  }
}

void MenuSystem::update(bool btnHeld, JoyDir dirEdge, unsigned long now) {
  // Edge: press started
  if (btnHeld && !_prevBtnHeld) {
    // A press while a single-tap is still pending (within the grace window)
    // is the second tap of a double-tap: while the menu is open, that's a
    // cancel (close without picking anything); while closed, it's the
    // active mode's contextual click.
    if (_pendingSingleTap && (now - _tapReleaseMs) < DOUBLE_TAP_WINDOW_MS) {
      _pendingSingleTap = false;
      _suppressNextRelease = true;
      if (_state == State::OPEN) {
        _state = State::CLOSED;
      } else {
        _clickPending = true;
      }
    }
    _pressStartMs = now;
  }

  if (_state == State::OPEN && dirEdge != JoyDir::NONE) {
    if (dirEdge == JoyDir::UP) {
      _selectedIndex = (_selectedIndex == 0) ? (MODE_COUNT - 1) : (_selectedIndex - 1);
    } else if (dirEdge == JoyDir::DOWN) {
      _selectedIndex = (_selectedIndex + 1) % MODE_COUNT;
    }
  }

  // Edge: release -> defer the tap's action in case a second tap follows.
  if (!btnHeld && _prevBtnHeld) {
    if (_suppressNextRelease) {
      _suppressNextRelease = false;
    } else {
      _pendingSingleTap = true;
      _pendingActionState = _state;
      _tapReleaseMs = now;
    }
  }

  // Grace window elapsed with no second tap: commit the single-tap action.
  if (_pendingSingleTap && (now - _tapReleaseMs) >= DOUBLE_TAP_WINDOW_MS) {
    _pendingSingleTap = false;
    commitAction(_pendingActionState, now);
  }

  _prevBtnHeld = btnHeld;
}
