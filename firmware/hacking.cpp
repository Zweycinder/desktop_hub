#include "hacking.h"
#include <LiquidCrystal_I2C.h>

void HackingMode::enter() {
  _scrollOffset = 0;
  _progress = 0;
  _lastTickMs = millis();
}

static char hashHexDigit(uint32_t pos) {
  uint32_t x = pos * 2654435761UL;
  return "0123456789ABCDEF"[(x >> 28) & 0xF];
}

void HackingMode::render(LiquidCrystal_I2C& lcd, const Protocol& proto, unsigned long now) {
  if (now - _lastTickMs >= 100) {
    _lastTickMs = now;
    _scrollOffset++;
    _progress = (_progress + 1) % 101;
  }

  char line0[17];
  for (uint8_t i = 0; i < 16; i++) line0[i] = hashHexDigit(_scrollOffset + i);
  line0[16] = '\0';
  lcd.setCursor(0, 0);
  lcd.print(line0);

  uint8_t filled = _progress * 12 / 100;
  char line1[17];
  line1[0] = '[';
  for (uint8_t i = 0; i < 12; i++) line1[1 + i] = (i < filled) ? '=' : ' ';
  snprintf(line1 + 13, 4, "]%2u", min(_progress, (uint8_t)99));
  line1[16] = '\0';
  lcd.setCursor(0, 1);
  lcd.print(line1);
}
