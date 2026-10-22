#include "dino.h"
#include <LiquidCrystal_I2C.h>

#define GLYPH_RUN1  0
#define GLYPH_RUN2  1
#define GLYPH_DUCK  2
#define GLYPH_JUMP  3
#define GLYPH_CACTUS 4
#define GLYPH_BIRD  5

static const uint8_t GLYPHS[6][8] PROGMEM = {
  {0b00000, 0b00110, 0b00111, 0b01110, 0b11110, 0b01100, 0b01010, 0b00000}, // RUN1
  {0b00000, 0b00110, 0b00111, 0b01110, 0b11110, 0b01100, 0b00110, 0b00000}, // RUN2
  {0b00000, 0b00000, 0b00000, 0b01111, 0b11111, 0b00000, 0b00000, 0b00000}, // DUCK
  {0b00000, 0b00110, 0b00111, 0b01111, 0b11111, 0b00000, 0b00000, 0b00000}, // JUMP
  {0b00000, 0b00100, 0b01110, 0b10101, 0b00100, 0b00100, 0b00100, 0b00000}, // CACTUS
  {0b00000, 0b00000, 0b01010, 0b10101, 0b01110, 0b00000, 0b00000, 0b00000}, // BIRD
};

void DinoMode::enter(LiquidCrystal_I2C& lcd) {
  uint8_t buf[8];
  for (uint8_t i = 0; i < 6; i++) {
    memcpy_P(buf, GLYPHS[i], 8);
    lcd.createChar(i, buf);
  }
  randomSeed(analogRead(A5));
  restart();
}

void DinoMode::restart() {
  _dinoState = STANDING;
  _gameOver = false;
  _score = 0;
  _scrollIntervalMs = DINO_START_INTERVAL_MS;
  _lastScrollMs = millis();
  _lastAnimMs = millis();
  _animFrame = false;
  _obstacles[0] = {-1, 0, false};
  _obstacles[1] = {-1, 0, false};
}

void DinoMode::onDir(JoyDir d) {
  if (_gameOver) return;
  if (d == JoyDir::UP) {
    if (_dinoState != JUMPING) {
      _dinoState = JUMPING;
      _jumpStartMs = millis();
    }
  } else if (d == JoyDir::DOWN) {
    if (_dinoState == STANDING) {
      _dinoState = DUCKING;
      _duckUntilMs = millis() + DINO_DUCK_MS;
    }
  }
}

void DinoMode::onClick(Protocol& proto) {
  if (_gameOver) restart();
}

bool DinoMode::checkCollision() const {
  uint8_t dinoRow = (_dinoState == JUMPING) ? 0 : 1;
  for (uint8_t i = 0; i < 2; i++) {
    if (!_obstacles[i].active || _obstacles[i].col != DINO_COL) continue;
    uint8_t obstacleRow = (_obstacles[i].type == 1) ? 0 : 1;
    if (obstacleRow == dinoRow) return true;
  }
  return false;
}

void DinoMode::spawnMaybe() {
  bool colsClear = true;
  for (uint8_t i = 0; i < 2; i++) {
    if (_obstacles[i].active && _obstacles[i].col >= 12) colsClear = false;
  }
  if (!colsClear) return;

  for (uint8_t i = 0; i < 2; i++) {
    if (!_obstacles[i].active && random(0, 100) < 55) {
      _obstacles[i].active = true;
      _obstacles[i].col = 15;
      _obstacles[i].type = (random(0, 100) < 25) ? 1 : 0; // 25% birds, 75% cacti
      return;
    }
  }
}

void DinoMode::update(unsigned long now) {
  if (_gameOver) return;

  if (_dinoState == JUMPING && now - _jumpStartMs >= DINO_JUMP_MS) {
    _dinoState = STANDING;
  }
  if (_dinoState == DUCKING && now >= _duckUntilMs) {
    _dinoState = STANDING;
  }

  if (now - _lastAnimMs >= 150) {
    _lastAnimMs = now;
    _animFrame = !_animFrame;
  }

  if (now - _lastScrollMs >= _scrollIntervalMs) {
    _lastScrollMs = now;
    for (uint8_t i = 0; i < 2; i++) {
      if (_obstacles[i].active) {
        _obstacles[i].col--;
        if (_obstacles[i].col < 0) _obstacles[i].active = false;
      }
    }
    spawnMaybe();
    _score++;
    if (_score % DINO_SPEEDUP_EVERY == 0 && _scrollIntervalMs > DINO_MIN_INTERVAL_MS) {
      _scrollIntervalMs--;
    }
    if (checkCollision()) _gameOver = true;
  }
}

void DinoMode::render(LiquidCrystal_I2C& lcd, const Protocol& proto, unsigned long now) {
  if (_gameOver) {
    char line1[17];
    snprintf(line1, sizeof(line1), "Game Over! %4u", _score);
    for (uint8_t i = strlen(line1); i < 16; i++) line1[i] = ' ';
    line1[16] = '\0';
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(F("Dbl-tap: retry  "));
    return;
  }

  uint8_t dinoRow = (_dinoState == JUMPING) ? 0 : 1;
  uint8_t dinoGlyph;
  if (_dinoState == JUMPING) dinoGlyph = GLYPH_JUMP;
  else if (_dinoState == DUCKING) dinoGlyph = GLYPH_DUCK;
  else dinoGlyph = _animFrame ? GLYPH_RUN1 : GLYPH_RUN2;

  lcd.setCursor(0, 0);
  for (int8_t col = 0; col < 16; col++) {
    if (col == DINO_COL && dinoRow == 0) { lcd.write((uint8_t)dinoGlyph); continue; }
    bool drew = false;
    for (uint8_t i = 0; i < 2; i++) {
      if (_obstacles[i].active && _obstacles[i].col == col && _obstacles[i].type == 1) {
        lcd.write((uint8_t)GLYPH_BIRD);
        drew = true;
        break;
      }
    }
    if (!drew) lcd.print(' ');
  }

  lcd.setCursor(0, 1);
  for (int8_t col = 0; col < 16; col++) {
    if (col == DINO_COL && dinoRow == 1) { lcd.write((uint8_t)dinoGlyph); continue; }
    bool drew = false;
    for (uint8_t i = 0; i < 2; i++) {
      if (_obstacles[i].active && _obstacles[i].col == col && _obstacles[i].type == 0) {
        lcd.write((uint8_t)GLYPH_CACTUS);
        drew = true;
        break;
      }
    }
    if (!drew) lcd.print('_');
  }
}
