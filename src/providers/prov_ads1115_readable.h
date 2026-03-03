#pragma once
/**
 * @file prov_ads1115_readable.h
 * @brief ADS1115 analog input provider (threshold + hysteresis) -> RawInputEvent.
 *
 * Why this exists
 * ---------------
 * Some alarm loops / sensors are analog (EOL resistors, voltage ladders, etc.).
 * This provider:
 *   - Reads ADS1115 single-ended channels (0..3)
 *   - Converts ADC counts to volts
 *   - Applies threshold + hysteresis to produce a stable digital level (0/1)
 *   - Emits RawInputEvent on changes (only when the derived level changes)
 *
 * Notes
 * -----
 * - Polling is round-robin: one (device,channel) per poll() call.
 * - Supports up to 4 ADS1115 devices, each with 4 channels (total 16 inputs).
 * - Gain is set to GAIN_TWOTHIRDS by default (±6.144V range). Adjust if needed.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include "../event_bus.h"  // RawInputEvent, rawPush(), RAWF_ANALOG

/**
 * @brief Per-channel threshold configuration.
 *
 * The level output is:
 *   - 0 (inactive) until voltage >= thresholdHighVolts
 *   - then 1 (active) until voltage <= thresholdLowVolts
 *
 * This is classic hysteresis which prevents jitter around the threshold.
 */
struct AdsChannelConfig {
  uint8_t zoneId;             ///< Logical zone id to emit events for
  uint8_t channelIndex;       ///< ADS1115 channel: 0..3
  float thresholdHighVolts;   ///< Rising threshold (0->1)
  float thresholdLowVolts;    ///< Falling threshold (1->0)
  bool enabled;               ///< Channel enabled flag
};

/**
 * @brief Per-ADS1115 device config.
 */
struct AdsDeviceConfig {
  uint8_t i2cAddress;         ///< ADS1115 address: 0x48..0x4B
  AdsChannelConfig channels[4];
};

/**
 * @brief Provider that samples ADS1115 channels and emits RawInputEvent on level changes.
 */
class ProvADS1115 {
public:
  /**
   * @brief Initialize up to 4 ADS1115 devices.
   * @param devices Pointer to device array (must remain valid after begin()).
   * @param deviceCount Number of devices (clamped to 4).
   * @return true if initialization routine completed (individual devices may still fail and will be skipped).
   */
  bool begin(const AdsDeviceConfig* devices, uint8_t deviceCount);

  /**
   * @brief Poll one channel (round-robin). Call periodically.
   */
  void poll();

  /**
   * @brief Set the minimum time between poll steps (one step = one channel read).
   */
  void setPollIntervalMs(uint16_t milliseconds) { pollIntervalMs_ = milliseconds; }

private:
  const AdsDeviceConfig* devices_ = nullptr;
  uint8_t deviceCount_ = 0;

  // Hardware objects + per-device availability
  Adafruit_ADS1115 ads_[4];
  bool deviceOk_[4] = {false, false, false, false};

  // Derived digital level per device/channel (0 or 1)
  uint8_t channelLevel_[4][4] = {{0}};

  // Round-robin pointers
  uint8_t nextDeviceIndex_ = 0;
  uint8_t nextChannelIndex_ = 0;

  // Poll timing
  uint32_t lastPollMs_ = 0;
  uint16_t pollIntervalMs_ = 15; ///< default step time (ms)
};
