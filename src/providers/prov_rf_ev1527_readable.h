#pragma once
/**
 * @file prov_rf_ev1527_readable.h
 * @brief RF provider for EV1527-style codes -> RawInputEvent.
 *
 * What it does
 * ------------
 * - You feed decoded EV1527 `code` values into onCode().
 * - It looks up the code in a mapping table and emits a RawInputEvent for the mapped zone.
 * - It blocks repeated identical codes within repeatBlockMs (simple debouncing / repeat filter).
 *
 * Default behavior
 * ----------------
 * Emits a pulse event with level=1 (OPEN). Your ZoneEngine can auto-close using momentary_hold_ms.
 */

#include <Arduino.h>
#include "../event_bus.h"  // RawInputEvent, rawPush(), RAWF_RF

/**
 * @brief Map an RF code to a zone id.
 */
struct RfCodeToZone {
  uint32_t code;  ///< EV1527 decoded code
  uint8_t zoneId; ///< target zone
};

/**
 * @brief RF EV1527 provider (code->zone) with repeat filtering.
 */
class ProvRF_EV1527 {
public:
  /**
   * @brief Provide the mapping table.
   * @param map Pointer to map array (must remain valid after begin()).
   * @param count Number of entries in map.
   */
  void begin(const RfCodeToZone* map, uint8_t count);

  /**
   * @brief Call this when your RF decoder receives a code.
   * @param code EV1527 decoded code.
   */
  void onCode(uint32_t code);

  /**
   * @brief Set minimum time to ignore identical repeated codes (ms).
   */
  void setRepeatBlockMs(uint16_t ms) { repeatBlockMs_ = ms; }

private:
  const RfCodeToZone* map_ = nullptr;
  uint8_t mapCount_ = 0;

  uint32_t lastCode_ = 0;
  uint32_t lastCodeMs_ = 0;
  uint16_t repeatBlockMs_ = 250;
};
