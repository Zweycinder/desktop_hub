#ifndef HACKING_H
#define HACKING_H

#include <Arduino.h>
#include "config.h"
#include "input.h"
#include "protocol.h"
#include "mode.h"

class HackingMode : public Mode {
  public:
    void enter();

    void update(unsigned long now) override {}
    void render(LiquidCrystal_I2C& lcd, const Protocol& proto, unsigned long now) override;

  private:
    uint32_t _scrollOffset = 0;
    uint8_t _progress = 0;
    unsigned long _lastTickMs = 0;
};

#endif
