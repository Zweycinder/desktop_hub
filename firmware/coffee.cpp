#include "coffee.h"
#include <LiquidCrystal_I2C.h>

void CoffeeMode::enter() {
  randomSeed(analogRead(A5));
  _state = IDLE;
}

void CoffeeMode::onClick(Protocol& proto) {
  if (_state != FLIPPING) {
    _state = FLIPPING;
    _getCoffee = random(0, 2) == 0;
    _stateUntilMs = millis() + 900;
  }
}

void CoffeeMode::render(LiquidCrystal_I2C& lcd, const Protocol& proto, unsigned long now) {
  if (_state == FLIPPING && now >= _stateUntilMs) {
    _state = RESULT;
    _stateUntilMs = now + 3000;
  } else if (_state == RESULT && now >= _stateUntilMs) {
    _state = IDLE;
  }

  switch (_state) {
    case IDLE:
      lcd.setCursor(0, 0);
      lcd.print(F("Another coffee? "));
      lcd.setCursor(0, 1);
      lcd.print(F("Dbl-tap to flip "));
      break;
    case FLIPPING:
      lcd.setCursor(0, 0);
      lcd.print(F("*flipping*      "));
      lcd.setCursor(0, 1);
      lcd.print(F("                "));
      break;
    case RESULT:
      lcd.setCursor(0, 0);
      lcd.print(_getCoffee ? F("Yes - get coffee") : F("No - skip it    "));
      lcd.setCursor(0, 1);
      lcd.print(F("                "));
      break;
  }
}
