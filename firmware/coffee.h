#ifndef COFFEE_H
#define COFFEE_H

#include <Arduino.h>
#include "config.h"
#include "input.h"
#include "protocol.h"
#include "mode.h"

class CoffeeMode : public Mode {
  public:
    void enter();

    void onClick(Protocol& proto) override;
    void update(unsigned long now) override {}
    void render(LiquidCrystal_I2C& lcd, const Protocol& proto, unsigned long now) override;

  private:
    enum State : uint8_t { IDLE, FLIPPING, RESULT };

    State _state = IDLE;
    bool _getCoffee = false;
    unsigned long _stateUntilMs = 0;
};

#endif
