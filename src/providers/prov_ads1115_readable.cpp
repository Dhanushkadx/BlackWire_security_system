/**
 * @file prov_ads1115_readable.cpp
 * @brief Implementation of ProvADS1115.
 */

#include "prov_ads1115_readable.h"

/**
 * Convert ADS1115 counts to volts.
 * Assumes gain = GAIN_TWOTHIRDS (full-scale = 6.144V).
 */
static float adcCountsToVolts(int16_t counts) {
  return (float)counts * (6.144f / 32768.0f);
}

bool ProvADS1115::begin(const AdsDeviceConfig* devices, uint8_t deviceCount) {
  devices_ = devices;
  deviceCount_ = deviceCount;
  if (deviceCount_ > 4) deviceCount_ = 4;

  // Initialize each ADS1115 device (if present)
  for (uint8_t i = 0; i < deviceCount_; i++) {
    deviceOk_[i] = ads_[i].begin(devices_[i].i2cAddress);
    if (deviceOk_[i]) {
      // Adjust gain if your voltage divider range differs
      ads_[i].setGain(GAIN_TWOTHIRDS);

      // Reset derived levels
      for (uint8_t ch = 0; ch < 4; ch++) {
        channelLevel_[i][ch] = 0;
      }
    }
  }

  nextDeviceIndex_ = 0;
  nextChannelIndex_ = 0;
  lastPollMs_ = millis();
  return true;
}

void ProvADS1115::poll() {
  if (!devices_ || deviceCount_ == 0) return;

  const uint32_t now = millis();
  if ((uint32_t)(now - lastPollMs_) < pollIntervalMs_) return;
  lastPollMs_ = now;

  // Snapshot current indices for this step, then advance for next step
  const uint8_t deviceIndex = nextDeviceIndex_;
  const uint8_t channelIndex = nextChannelIndex_;

  nextChannelIndex_++;
  if (nextChannelIndex_ >= 4) {
    nextChannelIndex_ = 0;
    nextDeviceIndex_++;
    if (nextDeviceIndex_ >= deviceCount_) nextDeviceIndex_ = 0;
  }

  // Skip invalid/unavailable devices
  if (deviceIndex >= deviceCount_) return;
  if (!deviceOk_[deviceIndex]) return;

  const AdsChannelConfig& cfg = devices_[deviceIndex].channels[channelIndex];
  if (!cfg.enabled) return;

  // Read ADC, convert to volts
  const int16_t rawCounts = ads_[deviceIndex].readADC_SingleEnded(channelIndex);
  const float volts = adcCountsToVolts(rawCounts);

  // Apply hysteresis to derive stable digital level
  uint8_t newLevel = channelLevel_[deviceIndex][channelIndex];
  if (newLevel == 0) {
    if (volts >= cfg.thresholdHighVolts) newLevel = 1;
  } else {
    if (volts <= cfg.thresholdLowVolts) newLevel = 0;
  }

  // Emit only on change
  if (newLevel != channelLevel_[deviceIndex][channelIndex]) {
    channelLevel_[deviceIndex][channelIndex] = newLevel;
    rawPush(RawInputEvent{ cfg.zoneId, newLevel, (uint8_t)(RAWF_ANALOG), now });
  }
}
