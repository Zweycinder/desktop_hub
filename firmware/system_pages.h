#ifndef MODES_SYSTEM_PAGES_H
#define MODES_SYSTEM_PAGES_H

#include <Arduino.h>
#include "config.h"
#include "input.h"
#include "protocol.h"
#include "mode.h"

class SystemPagesMode : public Mode {
  public:
    static const uint8_t PAGE_CPU = 0;
    static const uint8_t PAGE_RAM = 1;
    static const uint8_t PAGE_GPU = 2;
    static const uint8_t PAGE_TEMPS = 3;
    static const uint8_t PAGE_PING = 4;
    static const uint8_t PAGE_NETWORK = 5;
    static const uint8_t PAGE_COUNT = 6;

    void enter(uint8_t startPage, const Protocol& proto); // startPage==PAGE_COUNT means auto-cycle
    void onDir(JoyDir d) override;
    void onClick(Protocol& proto) override;
    void update(unsigned long now) override;

    void render(LiquidCrystal_I2C& lcd, const Protocol& proto, unsigned long now) override;

  private:
    uint8_t _page = 0;
    bool _autoCycling = false;
    unsigned long _lastAutoStepMs = 0;
};

#endif
