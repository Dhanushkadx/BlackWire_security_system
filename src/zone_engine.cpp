#include "zone_engine.h"


// ---------------------------------------------------------------------------
void ZoneEngine::begin() {
  uint32_t now = millis();
  for (uint8_t i = 0; i < ZONE_COUNT; i++) {
    ZoneRuntime& rt   = zones_[i];
    rt.state          = ZS_CLOSE;
    rt.rawLevel       = 0;
    rt.stableLevel    = 0;
    rt.rawChangedAt   = now;
    rt.stableChangedAt = now;
    rt.momentaryDeadline = 0;
  }
}

void ZoneEngine::setConfig(uint8_t zoneId, const ZoneConfig& cfg) {
  if (zoneId >= ZONE_COUNT) return;
  zones_[zoneId].cfg = cfg;
}

ZoneConfig ZoneEngine::getConfig(uint8_t zoneId) const {
  if (zoneId >= ZONE_COUNT) return ZoneConfig{};
  return zones_[zoneId].cfg;
}

ZoneState ZoneEngine::getState(uint8_t zoneId) const {
  if (zoneId >= ZONE_COUNT) return ZS_FAULT;
  return zones_[zoneId].state;
}

void ZoneEngine::emitIfChanged(uint8_t zoneId, ZoneState newState, uint32_t nowMs) {
  if (zoneId >= ZONE_COUNT) return;
  if (zones_[zoneId].state == newState) return;
  zones_[zoneId].state = newState;
  Serial.printf("[ZE] Zone %u state → %s\n", zoneId,
    newState == ZS_OPEN  ? "OPEN"  :
    newState == ZS_CLOSE ? "CLOSE" : "FAULT");
  zonePush(ZoneEvent{ zoneId, newState, nowMs });
}


// ---------------------------------------------------------------------------
// onRaw — called for every raw hardware edge from any provider
// ---------------------------------------------------------------------------
void ZoneEngine::onRaw(const RawInputEvent& ev) {
  if (ev.zone >= ZONE_COUNT) return;

  ZoneRuntime& rt = zones_[ev.zone];

  // --- Bypassed zone: track raw but only allow FAULT, never OPEN ----------
  if (rt.cfg.bypass) {
    Serial.println(F("[ZE] Bypassed zone event"));
    rt.rawLevel     = ev.level;
    rt.rawChangedAt = ev.t_ms;
    if (ev.flags & RAWF_FAULT) emitIfChanged(ev.zone, ZS_FAULT, ev.t_ms);
    else                       emitIfChanged(ev.zone, ZS_CLOSE, ev.t_ms);
    return;
  }

  // --- Provider-level fault (e.g. ADS1115 EOL short/open) -----------------
  if (ev.flags & RAWF_FAULT) {
    Serial.println(F("[ZE] Provider fault event"));
    rt.rawLevel     = ev.level;
    rt.rawChangedAt = ev.t_ms;
    emitIfChanged(ev.zone, ZS_FAULT, ev.t_ms);
    return;
  }

  // --- Raw edge ------------------------------------------------------------
  if (ev.level == rt.rawLevel) return; // no change, nothing to do

  Serial.println(F("[ZE] Raw level changed"));
  rt.rawLevel     = ev.level;
  rt.rawChangedAt = ev.t_ms;

  // If already in flap-fault, keep raw_level updated (needed when fault
  // clears) but otherwise ignore the edge — tick() will release the fault.
  if (rt.isFaulting) return;

  // --- Flap (chatter) detection -------------------------------------------
  // Record every raw edge BEFORE the debounce gate so rapid toggling
  // (faster than debounce_ms) is still caught.
  rt.flapBuf[rt.flapHead] = ev.t_ms;
  rt.flapHead = (rt.flapHead + 1) % ZONE_FLAP_COUNT;

  // After the write, flapHead points to the OLDEST entry in the buffer.
  const uint32_t oldestEdge = rt.flapBuf[rt.flapHead];
  const uint32_t windowAge  = ev.t_ms - oldestEdge;

  if (oldestEdge != 0 && windowAge <= ZONE_FLAP_WINDOW_MS) {
    // ZONE_FLAP_COUNT edges all fit inside the window → chattering
    rt.isFaulting    = true;
    rt.faultStartedAt = ev.t_ms;
    Serial.printf("[ZE] Zone %u FLAP FAULT: %u edges in %lums\n",
                  ev.zone, (unsigned)ZONE_FLAP_COUNT, (unsigned long)windowAge);
    emitIfChanged(ev.zone, ZS_FAULT, ev.t_ms);
    return;
  }

  // Debounce completion is handled in tick() — onRaw() cannot complete it
  // because rawChangedAt was just set to ev.t_ms, making elapsed always 0.
}


// ---------------------------------------------------------------------------
// tick — called periodically to finish debounce and handle timed events
// ---------------------------------------------------------------------------
void ZoneEngine::tick(uint32_t nowMs) {
  for (uint8_t zoneId = 0; zoneId < ZONE_COUNT; zoneId++) {
    ZoneRuntime& rt = zones_[zoneId];

    // --- Flap-fault hold and auto-clear ------------------------------------
    // The fault is held for at least ZONE_FLAP_HOLD_MS from when it was
    // first raised (faultStartedAt), regardless of subsequent activity.
    // Using a dedicated start timestamp means normal toggles during the
    // hold period do NOT reset the clear timer.
    if (rt.isFaulting) {
      if ((nowMs - rt.faultStartedAt) > ZONE_FLAP_HOLD_MS) {
        rt.isFaulting = false;
        // Wipe the ring buffer so the first clean edge after recovery
        // does not immediately re-trigger the fault.
        for (uint8_t j = 0; j < ZONE_FLAP_COUNT; j++) rt.flapBuf[j] = 0;
        rt.flapHead = 0;
        Serial.printf("[ZE] Zone %u flap fault cleared\n", zoneId);
        emitIfChanged(zoneId, rt.stableLevel ? ZS_OPEN : ZS_CLOSE, nowMs);
      }
      continue; // skip debounce and momentary while faulting
    }

    // --- Debounce completion -----------------------------------------------
    // GPIO and RF providers only emit events on edges, so the debounce timer
    // must be completed here rather than inside onRaw.
    if (rt.rawLevel != rt.stableLevel) {
      uint32_t elapsed = nowMs - rt.rawChangedAt;
      if (elapsed >= rt.cfg.debounce_ms) {
        Serial.printf("[ZE] Zone %u debounce passed (%ums)\n", zoneId, elapsed);
        rt.stableLevel    = rt.rawLevel;
        rt.stableChangedAt = nowMs;

        if (rt.cfg.momentary_hold_ms > 0 && rt.stableLevel == 1) {
          rt.momentaryDeadline = nowMs + rt.cfg.momentary_hold_ms;
        }

        emitIfChanged(zoneId, rt.stableLevel ? ZS_OPEN : ZS_CLOSE, nowMs);
      }
    }

    // --- Momentary auto-close ----------------------------------------------
    if (rt.cfg.momentary_hold_ms > 0 &&
        rt.state == ZS_OPEN          &&
        rt.momentaryDeadline != 0    &&
        (int32_t)(nowMs - rt.momentaryDeadline) >= 0)
    {
      rt.momentaryDeadline = 0;
      rt.stableLevel       = 0;
      rt.rawLevel          = 0;
      rt.rawChangedAt      = nowMs;
      rt.stableChangedAt   = nowMs;
      emitIfChanged(zoneId, ZS_CLOSE, nowMs);
    }
  }
}
