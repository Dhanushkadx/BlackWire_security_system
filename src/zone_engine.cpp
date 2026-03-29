#include "zone_engine.h"

void ZoneEngine::begin() {
  uint32_t now = millis();
  for (uint8_t i=0;i<ZONE_COUNT;i++){
    z[i].state = ZS_CLOSE;
    z[i].raw_level = 0;
    z[i].stable_level = 0;
    z[i].last_raw_change_ms = now;
    z[i].last_stable_change_ms = now;
    z[i].momentary_until_ms = 0;
  }
}

void ZoneEngine::setConfig(uint8_t zone, const ZoneConfig& cfg){
  if(zone >= ZONE_COUNT) return;
  z[zone].cfg = cfg;
}

ZoneConfig ZoneEngine::getConfig(uint8_t zone) const {
  ZoneConfig dummy;
  if(zone >= ZONE_COUNT) return dummy;
  return z[zone].cfg;
}

ZoneState ZoneEngine::getState(uint8_t zone) const {
  if(zone >= ZONE_COUNT) return ZS_FAULT;
  return z[zone].state;
}

void ZoneEngine::emitIfChanged(uint8_t zone, ZoneState newState, uint32_t now){
  if(zone >= ZONE_COUNT) return;
  if(z[zone].state == newState) return;
  z[zone].state = newState;
  zonePush( ZoneEvent{ zone, newState, now } );
}

void ZoneEngine::onRaw(const RawInputEvent& e){
  if(e.zone >= ZONE_COUNT) return;

  ZoneRuntime& r = z[e.zone];
  if(r.cfg.bypass){
    // Bypassed zones still track raw, but do not generate OPEN events
    Serial.println(F("Bypassed zone event"));
    r.raw_level = e.level;
    r.last_raw_change_ms = e.t_ms;
    // keep state CLOSE unless FAULT
    if(e.flags & RAWF_FAULT) emitIfChanged(e.zone, ZS_FAULT, e.t_ms);
    else emitIfChanged(e.zone, ZS_CLOSE, e.t_ms);
    return;
  }

  // If fault flag, go FAULT immediately
  if(e.flags & RAWF_FAULT){
    Serial.println(F("Fault event"));
    r.raw_level = e.level;
    r.last_raw_change_ms = e.t_ms;
    emitIfChanged(e.zone, ZS_FAULT, e.t_ms);
    return;
  }

  // Raw edge?
  if(e.level != r.raw_level){
    Serial.println(F("Raw level changed"));
    r.raw_level = e.level;
    r.last_raw_change_ms = e.t_ms;
  }

  // Debounce: only accept stable change if raw has stayed same for debounce_ms
  if(r.raw_level != r.stable_level){
    Serial.println(F("Checking debounce"));
    uint32_t dt = e.t_ms - r.last_raw_change_ms;
    if(dt >= r.cfg.debounce_ms){
      Serial.println(F("Debounce passed, updating stable level"));
      r.stable_level = r.raw_level;
      r.last_stable_change_ms = e.t_ms;

      // Momentary: if configured and we got OPEN, schedule auto-close
      if(r.cfg.momentary_hold_ms > 0 && r.stable_level == 1){
        Serial.println(F("Scheduling momentary auto-close"));
        r.momentary_until_ms = e.t_ms + r.cfg.momentary_hold_ms;
      }
      Serial.printf("Emitting state change: zone %u, level %u\n", e.zone, r.stable_level);
      emitIfChanged(e.zone, (r.stable_level ? ZS_OPEN : ZS_CLOSE), e.t_ms);
    }
  }
}

void ZoneEngine::tick(uint32_t now_ms){
  for(uint8_t i=0;i<ZONE_COUNT;i++){
    ZoneRuntime& r = z[i];

    // 1) Debounce completion check
    if (r.raw_level != r.stable_level) {
      uint32_t dt = now_ms - r.last_raw_change_ms;
      if (dt >= r.cfg.debounce_ms) {
        r.stable_level = r.raw_level;
        r.last_stable_change_ms = now_ms;

        // Momentary: if configured and we got OPEN, schedule auto-close
        if (r.cfg.momentary_hold_ms > 0 && r.stable_level == 1) {
          r.momentary_until_ms = now_ms + r.cfg.momentary_hold_ms;
        }

        emitIfChanged(i, (r.stable_level ? ZS_OPEN : ZS_CLOSE), now_ms);
      }
    }

    // 2) Momentary auto-close check
    if (r.cfg.momentary_hold_ms > 0 && r.state == ZS_OPEN && r.momentary_until_ms != 0) {
      if ((int32_t)(now_ms - r.momentary_until_ms) >= 0) {
        r.momentary_until_ms = 0;
        r.stable_level = 0;
        r.raw_level = 0;
        r.last_raw_change_ms = now_ms;
        r.last_stable_change_ms = now_ms;
        emitIfChanged(i, ZS_CLOSE, now_ms);
      }
    }
  }
}
