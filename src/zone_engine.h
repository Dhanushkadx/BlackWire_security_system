#pragma once
#include <Arduino.h>
#include "event_bus.h"

struct ZoneConfig {
  bool bypass = false;
  bool is24h  = false;
  bool chime  = false;     // UI uses sl as "Chime"
  uint16_t debounce_ms = 50;

  // Optional: for RF “momentary” open, auto-clear after hold time.
  // If 0 => no auto clear.
  uint16_t momentary_hold_ms = 0;
};

class ZoneEngine {
public:
  void begin();

  void setConfig(uint8_t zone, const ZoneConfig& cfg);
  ZoneConfig getConfig(uint8_t zone) const;

  ZoneState getState(uint8_t zone) const;

  // Called by engine task when it receives a raw event
  void onRaw(const RawInputEvent& e);

  // Called periodically (e.g., every 50ms) to handle momentary auto-clear timers
  void tick(uint32_t now_ms);

private:
  struct ZoneRuntime {
    ZoneConfig cfg;
    ZoneState state = ZS_CLOSE;

    uint8_t raw_level = 0;        // last raw input level (0/1)
    uint8_t stable_level = 0;     // debounced stable level

    uint32_t last_raw_change_ms = 0;
    uint32_t last_stable_change_ms = 0;

    // momentary handling
    uint32_t momentary_until_ms = 0;
  };

  ZoneRuntime z[ZONE_COUNT];

  void emitIfChanged(uint8_t zone, ZoneState newState, uint32_t now);
};
