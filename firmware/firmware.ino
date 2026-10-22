#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include "config.h"
#include "input.h"
#include "protocol.h"
#include "mode.h"
#include "system_pages.h"
#include "pet.h"
#include "visualizer.h"
#include "dino.h"
#include "hacking.h"
#include "wheel.h"
#include "coffee.h"

#define UNIX_TO_Y2K_OFFSET 946684800UL
#define MODE_RENDER_INTERVAL_MS 200

enum class ActiveApp : uint8_t { NONE, SYSTEM_PAGES, PET, VISUALIZER, DINO, HACKING, WHEEL, COFFEE };

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 16, 2);
ThreeWire rtcWire(PIN_RTC_DAT, PIN_RTC_CLK, PIN_RTC_RST);
RtcDS1302<ThreeWire> rtc(rtcWire);
JoystickReader joy;
MenuSystem menu;
Protocol protocol;
SystemPagesMode sysPages;
PetMode petMode;
VisualizerMode visMode;
DinoMode dinoMode;
HackingMode hackingMode;
WheelMode wheelMode;
CoffeeMode coffeeMode;

ActiveApp activeApp = ActiveApp::PET;
Mode* activeMode = &petMode;

bool btnHeldDebounced = false;
bool btnRawPrev = false;
unsigned long btnEdgeMs = 0;
unsigned long lastInputActivityMs = 0;

uint8_t cachedRtcHour = 12;
unsigned long lastRtcReadMs = 0;

MenuSystem::State lastRenderedMenuState = MenuSystem::State::CLOSED;
uint8_t lastRenderedMenuIndex = 255;
unsigned long lastModeRenderMs = 0;
bool lastBlinkOn = false;

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.print(F("PC Command Hub"));
  lcd.setCursor(0, 1);
  lcd.print(F("Calibrating..."));

  rtc.Begin();
  if (!rtc.IsDateTimeValid()) {
    rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));
  }
  if (rtc.GetIsWriteProtected()) {
    rtc.SetIsWriteProtected(false);
  }
  if (!rtc.GetIsRunning()) {
    rtc.SetIsRunning(true);
  }

  pinMode(PIN_JOY_SW, INPUT_PULLUP);

  joy.calibrate();
  menu.begin();
  protocol.begin();
  protocol.sendResyncRequest();

  cachedRtcHour = rtc.GetDateTime().Hour();
  petMode.enter();

  Serial.println(F("Ready"));
}

void renderMenu() {
  char name[12];
  copyModeName(menu.selectedIndex(), name, sizeof(name));

  char line0[17];
  snprintf(line0, sizeof(line0), "%-16s", name);
  lcd.setCursor(0, 0);
  lcd.print(line0);

  char line1[17];
  snprintf(line1, sizeof(line1), "%u/%u   tap:go", menu.selectedIndex() + 1, (unsigned)MODE_COUNT);
  for (uint8_t i = strlen(line1); i < 16; i++) line1[i] = ' ';
  line1[16] = '\0';
  lcd.setCursor(0, 1);
  lcd.print(line1);
}

void applyTimeSync() {
  uint32_t epoch;
  if (protocol.consumeTimeSync(&epoch)) {
    if (epoch > UNIX_TO_Y2K_OFFSET) {
      rtc.SetDateTime(RtcDateTime(epoch - UNIX_TO_Y2K_OFFSET));
    }
  }
}

void switchTo(ActiveApp app, int8_t modeId) {
  if (activeApp == ActiveApp::PET && app != ActiveApp::PET) {
    petMode.exit();
  }
  activeApp = app;
  switch (app) {
    case ActiveApp::SYSTEM_PAGES:
      activeMode = &sysPages;
      sysPages.enter((uint8_t)modeId, protocol);
      break;
    case ActiveApp::PET:
      activeMode = &petMode;
      petMode.enter();
      break;
    case ActiveApp::VISUALIZER:
      activeMode = &visMode;
      visMode.enter(lcd);
      break;
    case ActiveApp::DINO:
      activeMode = &dinoMode;
      dinoMode.enter(lcd);
      break;
    case ActiveApp::HACKING:
      activeMode = &hackingMode;
      hackingMode.enter();
      break;
    case ActiveApp::WHEEL:
      activeMode = &wheelMode;
      wheelMode.enter();
      break;
    case ActiveApp::COFFEE:
      activeMode = &coffeeMode;
      coffeeMode.enter();
      break;
    default:
      activeMode = nullptr;
      break;
  }
}

void loop() {
  unsigned long now = millis();

  protocol.update(now);
  applyTimeSync();

  if (now - lastRtcReadMs >= 1000) {
    lastRtcReadMs = now;
    // The DS1302's bit-banged link is noise-prone (likely picking up switching
    // noise from the nearby stepper driver); a single bad byte read can show
    // up as garbled digits or seconds jumping around. Only trust a reading
    // when two back-to-back reads agree; otherwise keep the last good value.
    RtcDateTime dt1 = rtc.GetDateTime();
    RtcDateTime dt2 = rtc.GetDateTime();
    if (dt1.TotalSeconds() == dt2.TotalSeconds()) {
      cachedRtcHour = dt1.Hour();
    }
  }

  // Debounce the joystick switch.
  bool btnRaw = digitalRead(PIN_JOY_SW) == LOW;
  if (btnRaw != btnRawPrev) {
    btnEdgeMs = now;
    btnRawPrev = btnRaw;
    lastInputActivityMs = now;
  }
  if (now - btnEdgeMs > BTN_DEBOUNCE_MS) {
    btnHeldDebounced = btnRaw;
  }

  JoyDir dirEdge = joy.update();
  if (dirEdge != JoyDir::NONE) lastInputActivityMs = now;
  menu.update(btnHeldDebounced, dirEdge, now);

  bool menuOpen = (menu.state() == MenuSystem::State::OPEN);

  if (!menuOpen && activeMode) {
    if (dirEdge != JoyDir::NONE) activeMode->onDir(dirEdge);
    if (menu.consumeClickEvent()) activeMode->onClick(protocol);
    activeMode->update(now);
  } else {
    menu.consumeClickEvent(); // discard clicks while the menu owns input
  }

  // Pet state keeps evolving in the background even when another mode is on screen.
  petMode.tick(now, protocol, cachedRtcHour, now - lastInputActivityMs);

  if (menu.consumeLaunchEvent()) {
    int8_t modeId = menu.lastModeId();
    protocol.sendMode((uint8_t)modeId);
    if (modeId == MODE_PERFORMANCE) {
      switchTo(ActiveApp::SYSTEM_PAGES, modeId);
    } else if (modeId == MODE_PET) {
      switchTo(ActiveApp::PET, modeId);
    } else if (modeId == MODE_VISUALIZER) {
      switchTo(ActiveApp::VISUALIZER, modeId);
    } else if (modeId == MODE_DINO) {
      switchTo(ActiveApp::DINO, modeId);
    } else if (modeId == MODE_HACKING) {
      switchTo(ActiveApp::HACKING, modeId);
    } else if (modeId == MODE_WHEEL) {
      switchTo(ActiveApp::WHEEL, modeId);
    } else if (modeId == MODE_COFFEE) {
      switchTo(ActiveApp::COFFEE, modeId);
    } else {
      switchTo(ActiveApp::NONE, modeId);
    }
  }

  // Re-render the menu only on state/selection changes (avoid flicker/tearing);
  // the active mode's own view refreshes on a timer since its data moves.
  MenuSystem::State mst = menu.state();
  bool menuNeedsRender = (mst != lastRenderedMenuState) ||
                          (mst == MenuSystem::State::OPEN && menu.selectedIndex() != lastRenderedMenuIndex);

  if (menuNeedsRender) {
    lastRenderedMenuState = mst;
    lastRenderedMenuIndex = menu.selectedIndex();

    if (mst == MenuSystem::State::OPEN) {
      renderMenu();
    } else if (!activeMode) {
      lcd.setCursor(0, 0);
      lcd.print(F("PC Command Hub  "));
      lcd.setCursor(0, 1);
      lcd.print(F("Ready           "));
    }
  }

  if (!menuOpen && activeMode && now - lastModeRenderMs >= MODE_RENDER_INTERVAL_MS) {
    lastModeRenderMs = now;
    activeMode->render(lcd, protocol, now);
  }

  // Alert overlay: flashes row 0 even over games/other modes when a temp or
  // ping/loss threshold is crossed. Only writes on blink transitions (not
  // every loop) to avoid spamming the I2C bus; when it turns off, the next
  // regular mode render (within MODE_RENDER_INTERVAL_MS) restores row 0.
  if (!menuOpen) {
    const SystemStats& s = protocol.stats();
    bool tempAlert = s.cpuTempC >= PET_PANIC_TEMP_C || s.gpuTempC >= PET_PANIC_TEMP_C;
    bool netAlert = s.pingMs >= PET_LAGGY_PING_MS || s.lossPct >= PET_LAGGY_LOSS_PCT;
    bool alertActive = protocol.isOnline(now) && (tempAlert || netAlert);
    bool blinkOn = alertActive && (((now / 600) % 2) == 0);
    if (blinkOn != lastBlinkOn) {
      lastBlinkOn = blinkOn;
      if (blinkOn) {
        lcd.setCursor(0, 0);
        lcd.print(tempAlert ? F("!! HIGH TEMP !! ") : F("!! NET ISSUE !! "));
      }
    }
  }
}
