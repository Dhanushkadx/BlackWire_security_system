/**
 * @file prov_rf_ev1527_readable.cpp
 * @brief Implementation of ProvRF_EV1527.
 */

#include "prov_rf_ev1527_readable.h"

void ProvRF_EV1527::begin(const RfCodeToZone* map, uint8_t count) {
  map_ = map;
  mapCount_ = count;
  lastCode_ = 0;
  lastCodeMs_ = 0;
}

void ProvRF_EV1527::onCode(uint32_t code) {
  const uint32_t now = millis();

  // Filter repeated identical codes arriving too quickly
  if (code == lastCode_ && (uint32_t)(now - lastCodeMs_) < repeatBlockMs_) {
    return;
  }
  lastCode_ = code;
  lastCodeMs_ = now;

  if (!map_ || mapCount_ == 0) return;

  // Find the code and emit event
  for (uint8_t i = 0; i < mapCount_; i++) {
    if (map_[i].code == code) {
      // Emit a pulse: level=1. ZoneEngine may auto-clear.
      rawPush(RawInputEvent{ map_[i].zoneId, 1, (uint8_t)(RAWF_RF), now });
      return;
    }
  }
}
