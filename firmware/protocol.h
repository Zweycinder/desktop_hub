#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <Arduino.h>

struct SystemStats {
  uint8_t cpuPct = 0, ramPct = 0, gpuPct = 0, vramPct = 0;
  int8_t cpuTempC = 0, gpuTempC = 0, ssdTempC = 0;
  int16_t pingMs = 0, jitterMs = 0;
  uint8_t lossPct = 0;
  uint16_t dlKBs = 0, ulKBs = 0;
  uint16_t peakKBs = 0;
  bool audioActive = false; // cheap "is music playing" flag, always sent regardless of mode
};

struct AudioFrame {
  uint8_t bands[16] = {0};
  unsigned long lastUpdateMs = 0;
};

class Protocol {
  public:
    void begin();
    void update(unsigned long now); // reads Serial, parses lines, updates _stats
    bool isOnline(unsigned long now) const;

    const SystemStats& stats() const { return _stats; }
    void resetPeak() { _stats.peakKBs = 0; }
    const AudioFrame& audio() const { return _audio; }

    void sendMode(uint8_t modeId);
    void sendResyncRequest();

    bool consumeTimeSync(uint32_t* outUnixEpoch);

  private:
    void handleLine(char* line);

    char _buf[60];
    uint8_t _len = 0;
    unsigned long _lastHeartbeatMs = 0;
    SystemStats _stats;
    AudioFrame _audio;
    bool _timeSyncPending = false;
    uint32_t _pendingEpoch = 0;
};

#endif
