#ifndef MODE_H
#define MODE_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "input.h"
#include "protocol.h"

class Mode {
  public:
    virtual void onDir(JoyDir d) {}
    virtual void onClick(Protocol& proto) {}
    virtual void update(unsigned long now) {}
    virtual void render(LiquidCrystal_I2C& lcd, const Protocol& proto, unsigned long now) = 0;
    virtual ~Mode() {}
};

#endif
