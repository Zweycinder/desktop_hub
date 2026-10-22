#include "protocol.h"

#define PC_OFFLINE_MS 5000

void Protocol::begin() {
  _len = 0;
}

bool Protocol::isOnline(unsigned long now) const {
  return _lastHeartbeatMs != 0 && (now - _lastHeartbeatMs) < PC_OFFLINE_MS;
}

void Protocol::sendMode(uint8_t modeId) {
  Serial.print(F("M,"));
  Serial.println(modeId);
}

void Protocol::sendResyncRequest() {
  Serial.println(F("R"));
}

bool Protocol::consumeTimeSync(uint32_t* outUnixEpoch) {
  if (_timeSyncPending) {
    _timeSyncPending = false;
    *outUnixEpoch = _pendingEpoch;
    return true;
  }
  return false;
}

static long nextField(char** cursor) {
  char* tok = strtok_r(nullptr, ",", cursor);
  return tok ? atol(tok) : 0;
}

void Protocol::handleLine(char* line) {
  if (line[0] == '\0') return;
  _lastHeartbeatMs = millis();

  char* cursor;
  char* first = strtok_r(line, ",", &cursor);
  if (!first) return;

  switch (first[0]) {
    case 'H':
      break; // heartbeat only, already timestamped above
    case 'T': {
      long epoch = nextField(&cursor);
      _pendingEpoch = (uint32_t)epoch;
      _timeSyncPending = true;
      break;
    }
    case 'Y':
      _stats.cpuPct   = (uint8_t)nextField(&cursor);
      _stats.ramPct   = (uint8_t)nextField(&cursor);
      _stats.gpuPct   = (uint8_t)nextField(&cursor);
      _stats.cpuTempC = (int8_t)nextField(&cursor);
      _stats.gpuTempC = (int8_t)nextField(&cursor);
      _stats.vramPct  = (uint8_t)nextField(&cursor);
      _stats.ssdTempC = (int8_t)nextField(&cursor);
      _stats.audioActive = nextField(&cursor) != 0;
      break;
    case 'N': {
      _stats.pingMs   = (int16_t)nextField(&cursor);
      _stats.jitterMs = (int16_t)nextField(&cursor);
      _stats.lossPct  = (uint8_t)nextField(&cursor);
      _stats.dlKBs    = (uint16_t)nextField(&cursor);
      _stats.ulKBs    = (uint16_t)nextField(&cursor);
      uint16_t peakCandidate = _stats.dlKBs > _stats.ulKBs ? _stats.dlKBs : _stats.ulKBs;
      if (peakCandidate > _stats.peakKBs) _stats.peakKBs = peakCandidate;
      break;
    }
    case 'A': {
      char* hexTok = strtok_r(nullptr, ",", &cursor);
      if (hexTok) {
        for (uint8_t i = 0; i < 16 && hexTok[i]; i++) {
          char c = hexTok[i];
          uint8_t v;
          if (c >= '0' && c <= '9') v = c - '0';
          else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
          else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
          else v = 0;
          _audio.bands[i] = v;
        }
        _audio.lastUpdateMs = millis();
      }
      break;
    }
    default:
      break; // E not handled until the secret modes / events milestone
  }
}

void Protocol::update(unsigned long now) {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      _buf[_len] = '\0';
      handleLine(_buf);
      _len = 0;
    } else if (c != '\r') {
      if (_len < sizeof(_buf) - 1) {
        _buf[_len++] = c;
      } else {
        _len = 0; // drop oversized/truncated line silently
      }
    }
  }
}
