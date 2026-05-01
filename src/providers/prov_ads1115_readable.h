#pragma once
/**
 * @file prov_ads1115_readable.h
 * @brief ADS1115 analog input provider with EOL fault detection and chip supervision.
 *
 * Each ADS1115 chip covers 4 zone channels. Two chips make one 8-zone expansion card.
 * Chips are supervised at boot and continuously during polling:
 *   - Boot: chip configured but not found -> ZS_FAULT pushed for all its zones.
 *   - Poll: ADS_MAX_MISSED_HEARTBEATS consecutive I2C failures -> ZS_FAULT.
 *   - Recovery: chip responds again -> fault cleared, normal polling resumes.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include "../event_bus.h"

// EOL fault thresholds (GAIN_TWOTHIRDS, +-6.144V range).
// SHORT: voltage collapses near 0V  (wires shorted together).
// OPEN:  voltage floats near rail   (wire broken / disconnected).
#define ADS_FAULT_SHORT_V         1.1f
#define ADS_FAULT_OPEN_V          5.0f

// Consecutive missed I2C heartbeats before a chip is declared lost.
#define ADS_MAX_MISSED_HEARTBEATS 3

struct AdsChannelConfig {
  uint8_t zoneId;
  uint8_t channelIndex;
  float   thresholdHighVolts;
  float   thresholdLowVolts;
  bool    enabled;
};

struct AdsDeviceConfig {
  uint8_t          i2cAddress;
  AdsChannelConfig channels[4];
};

class ProvADS1115 {
public:
  bool begin(const AdsDeviceConfig* devices, uint8_t deviceCount);
  void poll();
  void setPollIntervalMs(uint16_t ms) { pollIntervalMs_ = ms; }

private:
  const AdsDeviceConfig* devices_     = nullptr;
  uint8_t                deviceCount_ = 0;

  Adafruit_ADS1115 ads_[4];
  bool             deviceOk_[4]        = {false, false, false, false};
  uint8_t          missedHeartbeat_[4] = {0, 0, 0, 0};

  uint8_t  channelLevel_[4][4] = {{0}};
  bool     channelFault_[4][4] = {{false}};

  uint8_t  nextDeviceIndex_  = 0;
  uint8_t  nextChannelIndex_ = 0;
  uint32_t lastPollMs_       = 0;
  uint16_t pollIntervalMs_   = 15;

  bool chipAlive_(uint8_t deviceIndex) const;
  void faultAllZones_(uint8_t deviceIndex, uint32_t nowMs);
};