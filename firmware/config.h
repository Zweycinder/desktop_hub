#ifndef CONFIG_H
#define CONFIG_H

// ---- LCD (1602 with PCF8574 I2C backpack, not parallel) ----
// SDA/SCL are the hardware I2C pins on the Uno (A4/A5 double as these).
#define LCD_I2C_ADDR 0x27
// D2-D5, D11, D12 are free (no longer used for parallel LCD data/control).

// D8, D9, D10, D13 are free (stepper removed).

// ---- Joystick (HW-504) ----
// Single tap of the joystick switch opens/confirms the mode menu; double
// tap is the active mode's contextual click (feed pet, reset peak, etc).
#define PIN_JOY_X  A0
#define PIN_JOY_Y  A1
#define PIN_JOY_SW 7

// ---- RTC (DS1302) ----
#define PIN_RTC_CLK 6
#define PIN_RTC_DAT A2
#define PIN_RTC_RST A3

// A4, A5 free.

// ---- Joystick direction detection (hysteresis) ----
// Flip these after the first physical test if a direction reads backwards
// or swapped, rather than rewiring.
#define JOY_SWAP_XY   false
#define JOY_INVERT_LR false
#define JOY_INVERT_UD false
#define JOY_PUSH_THRESHOLD 250  // deflection from center that counts as a push
#define JOY_DEADZONE       120  // must return inside this before the next push arms

// ---- Joystick switch gestures ----
// Single tap is deferred by DOUBLE_TAP_WINDOW_MS to leave room for a second
// tap (double-tap = contextual click / cancel-menu).
#define BTN_DEBOUNCE_MS       30
#define DOUBLE_TAP_WINDOW_MS  1500  // 2nd press within this = double-tap click

// ---- Mode IDs ----
enum ModeId : uint8_t {
  MODE_PERFORMANCE = 0, // CPU/RAM/GPU/Temps/Ping/Network/Auto-cycle, flip with left/right
  MODE_VISUALIZER,
  MODE_DINO,
  MODE_PET,
  MODE_HACKING,
  MODE_WHEEL,
  MODE_COFFEE,
  MODE_COUNT
};

// ---- Desk pet ----
#define PET_NIGHT_START_HOUR 22  // RTC hour >= this or < PET_NIGHT_END_HOUR = night
#define PET_NIGHT_END_HOUR   7
#define PET_BORED_IDLE_MS    600000UL  // no joystick/button activity this long while awake+day = Bored
#define PET_PANIC_TEMP_C     85
#define PET_LAGGY_PING_MS    150
#define PET_LAGGY_LOSS_PCT   10
#define PET_STUFFED_RAM_PCT  90
#define PET_HUNGRY_THRESHOLD 70
#define PET_HUNGER_RATE_MS   60000UL   // +1 hunger per minute
#define PET_DRIFT_RATE_MS    30000UL   // +-1 happiness per 30s depending on mood
#define PET_FEED_HAPPINESS   15
#define PET_SAVE_INTERVAL_MS 600000UL  // EEPROM save every 10 minutes while active
#define PET_EEPROM_ADDR      0
#define PET_EEPROM_MAGIC     0xA5

// ---- Dino runner ----
#define DINO_COL              1
#define DINO_START_INTERVAL_MS 220
#define DINO_MIN_INTERVAL_MS  80
#define DINO_JUMP_MS          500
#define DINO_DUCK_MS          500
#define DINO_SPEEDUP_EVERY    10  // scroll ticks between each 1ms speedup

// ---- Wheel of fortune ----
#define WHEEL_SLOT_COUNT 8

enum PetMood : uint8_t {
  PET_MOOD_HAPPY = 0,
  PET_MOOD_ASLEEP,
  PET_MOOD_HUNGRY,
  PET_MOOD_PANICKED,
  PET_MOOD_EXCITED,
  PET_MOOD_BORED,
  PET_MOOD_LAGGY,
  PET_MOOD_STUFFED,
  PET_MOOD_COUNT
};

#endif
