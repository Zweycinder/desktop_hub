#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>
#include "config.h"

enum class JoyDir : uint8_t { NONE, UP, DOWN, LEFT, RIGHT };

class JoystickReader {
  public:
    void calibrate(); // average 16 samples at boot; stick must be centered
    JoyDir update();  // call every loop; returns a push edge, else NONE

  private:
    int _centerX = 512;
    int _centerY = 512;
    bool _armed = true;
};

void copyModeName(uint8_t modeId, char* out, uint8_t outLen);

// Full-screen scrollable menu: tap opens it / confirms a selection, up/down
// scroll, double-tap cancels (while open) or is passed through as the
// active mode's contextual click (while closed).
class MenuSystem {
  public:
    enum class State : uint8_t { CLOSED, OPEN };

    void begin();
    // btnHeld = debounced current physical state of the switch (true = pressed).
    // dirEdge = a freshly detected joystick push this loop, else JoyDir::NONE.
    void update(bool btnHeld, JoyDir dirEdge, unsigned long now);

    State state() const { return _state; }
    uint8_t selectedIndex() const { return _selectedIndex; }

    // True for one update() call right after a selection is confirmed.
    bool consumeLaunchEvent();
    int8_t lastModeId() const { return _lastModeId; }

    // True for one update() call right after a double-tap while closed.
    bool consumeClickEvent();

  private:
    void commitAction(State fromState, unsigned long now);

    State _state = State::CLOSED;
    bool _prevBtnHeld = false;
    unsigned long _pressStartMs = 0;

    bool _pendingSingleTap = false;
    State _pendingActionState = State::CLOSED;
    unsigned long _tapReleaseMs = 0;
    bool _clickPending = false;
    bool _suppressNextRelease = false;

    uint8_t _selectedIndex = 0;
    int8_t _lastModeId = -1;
    bool _launchPending = false;
};

#endif
