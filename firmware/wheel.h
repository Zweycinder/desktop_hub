#ifndef WHEEL_H
#define WHEEL_H

#include <Arduino.h>
#include "config.h"
#include "input.h"
#include "protocol.h"
#include "mode.h"

class WheelMode : public Mode {
  public:
    void enter();

    void onClick(Protocol& proto) override;
    void update(unsigned long now) override {}
    void render(LiquidCrystal_I2C& lcd, const Protocol& proto, unsigned long now) override;

  private:
    enum State : uint8_t { IDLE, SPINNING, RESULT };

    State _state = IDLE;
    uint8_t _targetSlot = 0;
    unsigned long _spinUntilMs = 0;
    unsigned long _resultUntilMs = 0;
};

#endif
