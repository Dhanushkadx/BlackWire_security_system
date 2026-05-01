#pragma once
#include <Arduino.h>
#include "event_bus.h"

// ---------------------------------------------------------------------------
// Flap (chatter) detection constants.
//
// A loose wire or noisy sensor toggles rapidly. If ZONE_FLAP_COUNT raw edges
// are seen within ZONE_FLAP_WINDOW_MS the zone is declared faulty.
// The fault is held for ZONE_FLAP_HOLD_MS after detection so a briefly-quiet
// noisy zone does not immediately recover.
// ---------------------------------------------------------------------------
#define ZONE_FLAP_COUNT      15      // edges inside the window that trigger fault
#define ZONE_FLAP_WINDOW_MS  10000   // detection window (ms)
#define ZONE_FLAP_HOLD_MS    10000   // minimum fault hold time after detection (ms)


struct ZoneConfig {
  bool     bypass          = false;
  bool     is24h           = false;
  bool     chime           = false;
  uint16_t debounce_ms     = 50;
  uint16_t momentary_hold_ms = 0;   // 0 = not momentary; >0 = auto-close after this many ms
};


class ZoneEngine {
public:
  void begin();

  void      setConfig(uint8_t zoneId, const ZoneConfig& cfg);
  ZoneConfig getConfig(uint8_t zoneId) const;
  ZoneState  getState (uint8_t zoneId) const;

  // Feed a raw hardware event into the engine (called by pollTask / RF task).
  void onRaw(const RawInputEvent& ev);

  // Periodic timer tick — handles debounce completion and momentary auto-close.
  void tick(uint32_t nowMs);

private:
  // -------------------------------------------------------------------------
  // Per-zone runtime state — everything needed to debounce and detect faults.
  // -------------------------------------------------------------------------
  struct ZoneRuntime {
    ZoneConfig cfg;
    ZoneState  state        = ZS_CLOSE;

    uint8_t  rawLevel       = 0;   // last raw level received from the provider (0/1)
    uint8_t  stableLevel    = 0;   // debounced level (updated only after debounce_ms)

    uint32_t rawChangedAt   = 0;   // millis() of the last raw edge
    uint32_t stableChangedAt = 0;  // millis() of the last stable-level change

    uint32_t momentaryDeadline = 0; // millis() when a momentary-open should auto-close

    // -----------------------------------------------------------------------
    // Flap detection ring buffer.
    //
    // Every raw edge is timestamped and stored here. After ZONE_FLAP_COUNT
    // edges, flapHead wraps and the slot we just wrote becomes the OLDEST
    // entry. If that oldest entry is still within ZONE_FLAP_WINDOW_MS,
    // all ZONE_FLAP_COUNT edges happened in the window → chatter fault.
    // -----------------------------------------------------------------------
    uint32_t flapBuf[ZONE_FLAP_COUNT] = {0}; // ring buffer of raw-edge timestamps
    uint8_t  flapHead                 = 0;   // next write index (wraps mod ZONE_FLAP_COUNT)
    bool     isFaulting               = false; // true while in flap-fault state
    uint32_t faultStartedAt           = 0;   // millis() when isFaulting was first raised
  };

  ZoneRuntime zones_[ZONE_COUNT];

  void emitIfChanged(uint8_t zoneId, ZoneState newState, uint32_t nowMs);
};
