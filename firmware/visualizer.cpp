#include "visualizer.h"
#include <LiquidCrystal_I2C.h>

// Classic 16x2 LCD VU-meter trick: 7 custom chars for partial bar heights
// (1-7 rows filled from the bottom), plus the built-in full block (byte 255)
// for a completely filled cell and a plain space for empty. Together with
// the empty/full cases that's 9 levels per cell, so two stacked cells (top
// row + bottom row) give 0-16 levels of height per column. These glyphs
// are static for the whole mode, defined once on enter() (not per-frame).
static const uint8_t BAR_GLYPHS[7][8] PROGMEM = {
  {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111}, // 1
  {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111}, // 2
  {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111}, // 3
  {0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111}, // 4
  {0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111}, // 5
  {0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111}, // 6
  {0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111}, // 7
};

void VisualizerMode::enter(LiquidCrystal_I2C& lcd) {
  memset(_peakHold, 0, sizeof(_peakHold));
  _lastPeakDecayMs = millis();

  // Defined once here, not per-frame: each createChar() is several slow
  // HD44780 command writes, and doing 7 of them every render tick was
  // blocking loop() long enough to swallow button presses.
  uint8_t glyphBuf[8];
  for (uint8_t i = 0; i < 7; i++) {
    memcpy_P(glyphBuf, BAR_GLYPHS[i], 8);
    lcd.createChar(i, glyphBuf);
  }
}

void VisualizerMode::onClick(Protocol& proto) {
  _style = (Style)((_style + 1) % STYLE_COUNT);
  _styleFlashUntilMs = millis() + 900;
}

const char* VisualizerMode::styleName(Style s) {
  switch (s) {
    case STYLE_BARS: return "Bars";
    case STYLE_PEAK_DOTS: return "Peak Hold";
    case STYLE_MIRRORED: return "Mirrored";
    default: return "";
  }
}

static void writeCell(LiquidCrystal_I2C& lcd, uint8_t height) {
  // height: 0-8 for one cell (one row of one column).
  if (height == 0) lcd.print(' ');
  else if (height >= 8) lcd.write((uint8_t)255); // built-in solid block
  else lcd.write((uint8_t)(height - 1));          // custom slots 0-6
}

void VisualizerMode::render(LiquidCrystal_I2C& lcd, const Protocol& proto, unsigned long now) {
  if (now - _lastPeakDecayMs >= 150) {
    _lastPeakDecayMs = now;
    for (uint8_t i = 0; i < 16; i++) {
      if (_peakHold[i] > 0) _peakHold[i]--;
    }
  }

  const AudioFrame& a = proto.audio();
  uint8_t top[16], bottom[16];

  for (uint8_t i = 0; i < 16; i++) {
    uint8_t level = a.bands[i]; // 0-15
    if (level > _peakHold[i]) _peakHold[i] = level;

    uint8_t drawLevel = level;
    if (_style == STYLE_MIRRORED && (i % 2) == 1) {
      drawLevel = a.bands[15 - i]; // mirror odd columns from the opposite end
    } else if (_style == STYLE_PEAK_DOTS) {
      drawLevel = _peakHold[i];
    }

    bottom[i] = min(drawLevel, (uint8_t)8);
    top[i] = drawLevel > 8 ? drawLevel - 8 : 0;
  }

  if (now < _styleFlashUntilMs) {
    char line[17];
    snprintf(line, sizeof(line), "Style: %s", styleName(_style));
    for (uint8_t i = strlen(line); i < 16; i++) line[i] = ' ';
    line[16] = '\0';
    lcd.setCursor(0, 0);
    lcd.print(line);
  } else {
    lcd.setCursor(0, 0);
    for (uint8_t i = 0; i < 16; i++) writeCell(lcd, top[i]);
  }

  lcd.setCursor(0, 1);
  for (uint8_t i = 0; i < 16; i++) writeCell(lcd, bottom[i]);
}
