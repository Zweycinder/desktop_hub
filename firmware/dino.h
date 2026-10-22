#ifndef DINO_H
#define DINO_H

#include <Arduino.h>
#include "config.h"
#include "input.h"
#include "protocol.h"
#include "mode.h"

class DinoMode : public Mode {
  public:
    void enter(LiquidCrystal_I2C& lcd); // defines custom glyphs once

    void onDir(JoyDir d) override;
    void onClick(Protocol& proto) override;
    void update(unsigned long now) override;
    void render(LiquidCrystal_I2C& lcd, const Protocol& proto, unsigned long now) override;

  private:
    enum DinoState : uint8_t { STANDING, DUCKING, JUMPING };
    struct Obstacle { int8_t col; uint8_t type; bool active; }; // type: 0=cactus(row1), 1=bird(row0)

    DinoState _dinoState = STANDING;
    unsigned long _jumpStartMs = 0;
    unsigned long _duckUntilMs = 0;
    bool _gameOver = false;
    uint16_t _score = 0;
    uint16_t _scrollIntervalMs = 220;
    unsigned long _lastScrollMs = 0;
    unsigned long _lastAnimMs = 0;
    bool _animFrame = false;
    Obstacle _obstacles[2] = {{-1, 0, false}, {-1, 0, false}};

    void restart();
    void spawnMaybe();
    bool checkCollision() const;
};

#endif
