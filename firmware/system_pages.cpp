#include "system_pages.h"
#include <LiquidCrystal_I2C.h>

#define AUTO_CYCLE_MS 3000

void SystemPagesMode::enter(uint8_t startPage, const Protocol& proto) {
  if (startPage >= PAGE_COUNT) {
    _autoCycling = true;
    _page = PAGE_CPU;
    _lastAutoStepMs = millis();
  } else {
    _autoCycling = false;
    _page = startPage;
  }
}

void SystemPagesMode::onDir(JoyDir d) {
  if (d == JoyDir::LEFT) {
    _autoCycling = false;
    _page = (_page + PAGE_COUNT - 1) % PAGE_COUNT;
  } else if (d == JoyDir::RIGHT) {
    _autoCycling = false;
    _page = (_page + 1) % PAGE_COUNT;
  }
}

void SystemPagesMode::onClick(Protocol& proto) {
  if (_page == PAGE_NETWORK) {
    proto.resetPeak();
  }
}

void SystemPagesMode::update(unsigned long now) {
  if (_autoCycling && now - _lastAutoStepMs >= AUTO_CYCLE_MS) {
    _lastAutoStepMs = now;
    _page = (_page + 1) % PAGE_COUNT;
  }
}


static void padLine(char* line) {
  uint8_t len = strlen(line);
  for (uint8_t i = len; i < 16; i++) line[i] = ' ';
  line[16] = '\0';
}

void SystemPagesMode::render(LiquidCrystal_I2C& lcd, const Protocol& proto, unsigned long now) {
  char line1[17];
  char line2[17];

  if (!proto.isOnline(now)) {
    snprintf(line1, sizeof(line1), "PC offline");
    snprintf(line2, sizeof(line2), "waiting...");
  } else {
    const SystemStats& s = proto.stats();
    switch (_page) {
      case PAGE_CPU:
        snprintf(line1, sizeof(line1), "CPU %3u%%", s.cpuPct);
        snprintf(line2, sizeof(line2), "Temp %dC", s.cpuTempC);
        break;
      case PAGE_RAM:
        snprintf(line1, sizeof(line1), "RAM %3u%%", s.ramPct);
        snprintf(line2, sizeof(line2), " ");
        break;
      case PAGE_GPU:
        snprintf(line1, sizeof(line1), "GPU %3u%% %dC", s.gpuPct, s.gpuTempC);
        snprintf(line2, sizeof(line2), "VRAM %3u%%", s.vramPct);
        break;
      case PAGE_TEMPS:
        snprintf(line1, sizeof(line1), "C%dC G%dC", s.cpuTempC, s.gpuTempC);
        snprintf(line2, sizeof(line2), "SSD %dC", s.ssdTempC);
        break;
      case PAGE_PING:
        snprintf(line1, sizeof(line1), "Ping %dms", s.pingMs);
        snprintf(line2, sizeof(line2), "Jit%d Loss%u%%", s.jitterMs, s.lossPct);
        break;
      case PAGE_NETWORK:
        snprintf(line1, sizeof(line1), "D%u U%u KB/s", s.dlKBs, s.ulKBs);
        snprintf(line2, sizeof(line2), "Peak %u KB/s", s.peakKBs);
        break;
      default:
        snprintf(line1, sizeof(line1), "---");
        snprintf(line2, sizeof(line2), "---");
        break;
    }
  }

  padLine(line1);
  padLine(line2);
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}
