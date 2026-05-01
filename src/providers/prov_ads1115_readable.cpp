#include "prov_ads1115_readable.h"

static float adcCountsToVolts(int16_t counts) {
  return (float)counts * (6.144f / 32768.0f);
}

bool ProvADS1115::chipAlive_(uint8_t deviceIndex) const {
  if (deviceIndex >= deviceCount_) return false;
  Wire.beginTransmission(devices_[deviceIndex].i2cAddress);
  return Wire.endTransmission() == 0;
}

void ProvADS1115::faultAllZones_(uint8_t deviceIndex, uint32_t nowMs) {
  if (deviceIndex >= deviceCount_) return;
  for (uint8_t ch = 0; ch < 4; ch++) {
    const AdsChannelConfig& cfg = devices_[deviceIndex].channels[ch];
    if (!cfg.enabled) continue;
    channelFault_[deviceIndex][ch] = true;
    rawPush(RawInputEvent{ cfg.zoneId, 0, (uint8_t)(RAWF_ANALOG | RAWF_FAULT), nowMs });
  }
}

bool ProvADS1115::begin(const AdsDeviceConfig* devices, uint8_t deviceCount) {
  devices_     = devices;
  deviceCount_ = deviceCount;
  if (deviceCount_ > 4) deviceCount_ = 4;

  const uint32_t now = millis();

  for (uint8_t i = 0; i < deviceCount_; i++) {
    deviceOk_[i] = ads_[i].begin(devices_[i].i2cAddress);
    if (deviceOk_[i]) {
      ads_[i].setGain(GAIN_TWOTHIRDS);
      for (uint8_t ch = 0; ch < 4; ch++) channelLevel_[i][ch] = 0;
      missedHeartbeat_[i] = 0;
      Serial.printf("[ADS] Chip 0x%02X OK\n", devices_[i].i2cAddress);
    } else {
      // Chip configured but not responding -- fault all its zones immediately.
      Serial.printf("[ADS] Chip 0x%02X NOT FOUND -- faulting zones\n", devices_[i].i2cAddress);
      faultAllZones_(i, now);
    }
  }

  nextDeviceIndex_  = 0;
  nextChannelIndex_ = 0;
  lastPollMs_       = now;
  return true;
}

void ProvADS1115::poll() {
  if (!devices_ || deviceCount_ == 0) return;

  const uint32_t now = millis();
  if ((uint32_t)(now - lastPollMs_) < pollIntervalMs_) return;
  lastPollMs_ = now;

  const uint8_t deviceIndex  = nextDeviceIndex_;
  const uint8_t channelIndex = nextChannelIndex_;

  nextChannelIndex_++;
  if (nextChannelIndex_ >= 4) {
    nextChannelIndex_ = 0;
    nextDeviceIndex_++;
    if (nextDeviceIndex_ >= deviceCount_) nextDeviceIndex_ = 0;
  }

  if (deviceIndex >= deviceCount_) return;

  // -----------------------------------------------------------------------
  // Heartbeat supervision: on the first channel of each chip, do a
  // lightweight I2C probe. ADS_MAX_MISSED_HEARTBEATS consecutive failures
  // declare the chip lost and fault all its zones.
  // -----------------------------------------------------------------------
  if (channelIndex == 0) {
    if (chipAlive_(deviceIndex)) {
      if (!deviceOk_[deviceIndex]) {
        deviceOk_[deviceIndex]        = true;
        missedHeartbeat_[deviceIndex] = 0;
        Serial.printf("[ADS] Chip 0x%02X recovered\n", devices_[deviceIndex].i2cAddress);
        for (uint8_t ch = 0; ch < 4; ch++) channelFault_[deviceIndex][ch] = false;
      }
      missedHeartbeat_[deviceIndex] = 0;
    } else {
      missedHeartbeat_[deviceIndex]++;
      if (missedHeartbeat_[deviceIndex] >= ADS_MAX_MISSED_HEARTBEATS && deviceOk_[deviceIndex]) {
        deviceOk_[deviceIndex] = false;
        Serial.printf("[ADS] Chip 0x%02X LOST after %u misses -- faulting zones\n",
                      devices_[deviceIndex].i2cAddress,
                      (unsigned)ADS_MAX_MISSED_HEARTBEATS);
        faultAllZones_(deviceIndex, now);
      }
    }
  }

  if (!deviceOk_[deviceIndex]) return;

  const AdsChannelConfig& cfg = devices_[deviceIndex].channels[channelIndex];
  if (!cfg.enabled) return;

  const int16_t rawCounts = ads_[deviceIndex].readADC_SingleEnded(channelIndex);
  const float   volts     = adcCountsToVolts(rawCounts);

  // EOL fault detection.
  const bool isFault  = (volts < ADS_FAULT_SHORT_V || volts > ADS_FAULT_OPEN_V);
  const bool wasFault = channelFault_[deviceIndex][channelIndex];

  if (isFault) {
    if (!wasFault) {
      channelFault_[deviceIndex][channelIndex] = true;
      Serial.printf("[ADS] Zone %u FAULT: %.2fV\n", cfg.zoneId, volts);
      rawPush(RawInputEvent{ cfg.zoneId, 0, (uint8_t)(RAWF_ANALOG | RAWF_FAULT), now });
    }
    return;
  }

  if (wasFault) {
    channelFault_[deviceIndex][channelIndex] = false;
    Serial.printf("[ADS] Zone %u fault CLEARED: %.2fV\n", cfg.zoneId, volts);
  }

  // Normal hysteresis: derive stable digital level.
  uint8_t newLevel = channelLevel_[deviceIndex][channelIndex];
  if (newLevel == 0) {
    if (volts >= cfg.thresholdHighVolts) newLevel = 1;
  } else {
    if (volts <= cfg.thresholdLowVolts)  newLevel = 0;
  }

  if (newLevel != channelLevel_[deviceIndex][channelIndex]) {
    channelLevel_[deviceIndex][channelIndex] = newLevel;
    rawPush(RawInputEvent{ cfg.zoneId, newLevel, (uint8_t)(RAWF_ANALOG), now });
  }
}