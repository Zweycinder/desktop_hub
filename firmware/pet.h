#ifndef PET_H
#define PET_H

#include <Arduino.h>
#include "config.h"
#include "input.h"
#include "protocol.h"
#include "mode.h"

class PetMode : public Mode {
  public:
    void enter();
    void exit();

    // Background simulation: call every loop regardless of which mode is
    // active, so the pet's state keeps evolving even off-screen.
    void tick(unsigned long now, const Protocol& proto, uint8_t rtcHour, unsigned long idleMs);

    void onDir(JoyDir d) override;
    void onClick(Protocol& proto) override;
    void update(unsigned long now) override {}
    void render(LiquidCrystal_I2C& lcd, const Protocol& proto, unsigned long now) override;

  private:
    static const uint8_t PAGE_FACE = 0;
    static const uint8_t PAGE_STATUS = 1;

    uint8_t _happiness = 50, _hunger = 0, _energy = 100;
    uint8_t _mood = PET_MOOD_HAPPY;
    uint8_t _page = PAGE_FACE;
    unsigned long _lastHungerTickMs = 0;
    unsigned long _lastDriftTickMs = 0;
    unsigned long _lastSaveMs = 0;
    uint8_t _lastRenderKey = 255;

    // Smoking is a stateless quirk: not part of the mood priority list, it
    // just occasionally interrupts a calm (Happy/Bored) daytime moment.
    bool _quirkActive = false;
    uint8_t _quirkType = 0; // 0 = smoking, 1 = coffee
    unsigned long _quirkUntilMs = 0;
    unsigned long _lastQuirkCheckMs = 0;

    // Idle animation: each mood blinks to an alt frame on its own rhythm
    // (quick blink for most, slow breathing for Asleep, rapid flicker for
    // Laggy) so the faces aren't static.
    bool _blinkOn = false;
    unsigned long _blinkChangeAtMs = 0;

    void renderFace(LiquidCrystal_I2C& lcd);
    void renderStatus(LiquidCrystal_I2C& lcd);

    void computeMood(const Protocol& proto, bool night);
    void loadFromEeprom();
    void saveToEeprom();
};

#endif
