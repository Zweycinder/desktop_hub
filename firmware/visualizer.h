#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <Arduino.h>
#include "config.h"
#include "input.h"
#include "protocol.h"
#include "mode.h"

class VisualizerMode : public Mode {
  public:
    void enter(LiquidCrystal_I2C& lcd); // defines the 7 bar glyphs once, not per-frame

    void onClick(Protocol& proto) override;
    void update(unsigned long now) override {}
    void render(LiquidCrystal_I2C& lcd, const Protocol& proto, unsigned long now) override;

  private:
    enum Style : uint8_t { STYLE_BARS = 0, STYLE_PEAK_DOTS, STYLE_MIRRORED, STYLE_COUNT };
    static const char* styleName(Style s);

    Style _style = STYLE_BARS;
    uint8_t _peakHold[16] = {0};       // for STYLE_PEAK_DOTS
    unsigned long _lastPeakDecayMs = 0;
    unsigned long _styleFlashUntilMs = 0;
};

#endif
