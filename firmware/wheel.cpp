#include "wheel.h"
#include <LiquidCrystal_I2C.h>
#include <avr/pgmspace.h>

static const char SLOT_0[] PROGMEM = "JACKPOT";
static const char SLOT_1[] PROGMEM = "TRY AGAIN";
static const char SLOT_2[] PROGMEM = "BONUS X2";
static const char SLOT_3[] PROGMEM = "SMALL WIN";
static const char SLOT_4[] PROGMEM = "BIG WIN";
static const char SLOT_5[] PROGMEM = "NOTHING";
static const char SLOT_6[] PROGMEM = "MYSTERY";
static const char SLOT_7[] PROGMEM = "FREE SPIN";
static const char* const SLOT_NAMES[WHEEL_SLOT_COUNT] PROGMEM = {
  SLOT_0, SLOT_1, SLOT_2, SLOT_3, SLOT_4, SLOT_5, SLOT_6, SLOT_7
};

#define SPIN_DURATION_MS 1400

void WheelMode::enter() {
  _state = IDLE;
}

void WheelMode::onClick(Protocol& proto) {
  if (_state != SPINNING) {
    _targetSlot = random(0, WHEEL_SLOT_COUNT);
    _state = SPINNING;
    _spinUntilMs = millis() + SPIN_DURATION_MS;
  }
}

void WheelMode::render(LiquidCrystal_I2C& lcd, const Protocol& proto, unsigned long now) {
  if (_state == SPINNING && now >= _spinUntilMs) {
    _state = RESULT;
    _resultUntilMs = now + 4000;
  } else if (_state == RESULT && now >= _resultUntilMs) {
    _state = IDLE;
  }

  switch (_state) {
    case IDLE:
      lcd.setCursor(0, 0);
      lcd.print(F("Wheel of Fortune"));
      lcd.setCursor(0, 1);
      lcd.print(F("Dbl-tap to spin "));
      break;
    case SPINNING: {
      // Cycles a little spinner glyph so it still feels alive without a needle.
      static const char spinner[] = { '|', '/', '-', '\\' };
      char line0[17];
      snprintf(line0, sizeof(line0), "Spinning... %c   ", spinner[(now / 150) % 4]);
      lcd.setCursor(0, 0);
      lcd.print(line0);
      lcd.setCursor(0, 1);
      lcd.print(F("                "));
      break;
    }
    case RESULT: {
      char nameBuf[11];
      strncpy_P(nameBuf, (const char*)pgm_read_word(&SLOT_NAMES[_targetSlot]), sizeof(nameBuf) - 1);
      nameBuf[sizeof(nameBuf) - 1] = '\0';
      lcd.setCursor(0, 0);
      lcd.print(F("You got:        "));
      char line1[17];
      snprintf(line1, sizeof(line1), "%-16s", nameBuf);
      lcd.setCursor(0, 1);
      lcd.print(line1);
      break;
    }
  }
}
