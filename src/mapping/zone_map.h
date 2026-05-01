#pragma once
#include <Arduino.h>
#include "../event_bus.h"

// Zone hardware source types.
// SRC_NONE   = no hardware behind this slot -> reports unavailable (0x03).
// SRC_MODBUS = zone on a Modbus DI expansion card; active only when that card is enabled.
enum SourceType : uint8_t {
  SRC_NONE   = 0,
  SRC_GPIO   = 1,
  SRC_ADS    = 2,
  SRC_RF     = 3,
  SRC_MODBUS = 4,
};

// Modbus card enable flags — index 0=card1(Z08-Z15) .. 4=card5(Z40-Z47).
// Initialised to false; prov_modbus.begin() sets them from systemConfig.mbCard[].
extern bool g_modbusCardEnabled[5];

struct ZoneSource {
  SourceType type;
  uint8_t    providerId;  // GPIO bank / ADS chip index / RF table index
  uint8_t    channel;     // GPIO bit / ADS channel 0..3 / RF entry index
  uint16_t   param;       // reserved
};

// Hardware topology table -- one entry per zone slot (0..47).
// GSM_MINI_BOARD_V3: 4 GPIO + 4 RF onboard. Up to 5 Modbus 8-zone cards.
//   zones  0- 3  GPIO onboard
//   zones  4- 7  RF onboard
//   zones  8-15  Modbus card 1 (slave 1), providerId=0, channels 0-7
//   zones 16-23  Modbus card 2 (slave 2), providerId=1, channels 0-7
//   zones 24-31  Modbus card 3 (slave 3), providerId=2, channels 0-7
//   zones 32-39  Modbus card 4 (slave 4), providerId=3, channels 0-7
//   zones 40-47  Modbus card 5 (slave 5), providerId=4, channels 0-7
static const ZoneSource zoneSrc[ZONE_COUNT] = {
  // zones 0-3: onboard GPIO
  {SRC_GPIO, 0, 0, 0}, {SRC_GPIO, 0, 1, 0}, {SRC_GPIO, 0, 2, 0}, {SRC_GPIO, 0, 3, 0},

  // zones 4-7: onboard RF slots
  {SRC_RF, 0, 0, 0}, {SRC_RF, 0, 1, 0}, {SRC_RF, 0, 2, 0}, {SRC_RF, 0, 3, 0},

  // zones 8-15: Modbus card 1 (providerId=0)
  {SRC_MODBUS,0,0,0},{SRC_MODBUS,0,1,0},{SRC_MODBUS,0,2,0},{SRC_MODBUS,0,3,0},
  {SRC_MODBUS,0,4,0},{SRC_MODBUS,0,5,0},{SRC_MODBUS,0,6,0},{SRC_MODBUS,0,7,0},

  // zones 16-23: Modbus card 2 (providerId=1)
  {SRC_MODBUS,1,0,0},{SRC_MODBUS,1,1,0},{SRC_MODBUS,1,2,0},{SRC_MODBUS,1,3,0},
  {SRC_MODBUS,1,4,0},{SRC_MODBUS,1,5,0},{SRC_MODBUS,1,6,0},{SRC_MODBUS,1,7,0},

  // zones 24-31: Modbus card 3 (providerId=2)
  {SRC_MODBUS,2,0,0},{SRC_MODBUS,2,1,0},{SRC_MODBUS,2,2,0},{SRC_MODBUS,2,3,0},
  {SRC_MODBUS,2,4,0},{SRC_MODBUS,2,5,0},{SRC_MODBUS,2,6,0},{SRC_MODBUS,2,7,0},

  // zones 32-39: Modbus card 4 (providerId=3)
  {SRC_MODBUS,3,0,0},{SRC_MODBUS,3,1,0},{SRC_MODBUS,3,2,0},{SRC_MODBUS,3,3,0},
  {SRC_MODBUS,3,4,0},{SRC_MODBUS,3,5,0},{SRC_MODBUS,3,6,0},{SRC_MODBUS,3,7,0},

  // zones 40-47: Modbus card 5 (providerId=4)
  {SRC_MODBUS,4,0,0},{SRC_MODBUS,4,1,0},{SRC_MODBUS,4,2,0},{SRC_MODBUS,4,3,0},
  {SRC_MODBUS,4,4,0},{SRC_MODBUS,4,5,0},{SRC_MODBUS,4,6,0},{SRC_MODBUS,4,7,0},
};

// True if this zone slot has hardware and (for Modbus) its card is enabled.
inline bool isZoneActive(uint8_t zoneId) {
  if (zoneId >= ZONE_COUNT) return false;
  const SourceType t = zoneSrc[zoneId].type;
  if (t == SRC_NONE) return false;
  if (t == SRC_MODBUS) return g_modbusCardEnabled[zoneSrc[zoneId].providerId];
  return true;
}

// True if this zone's source is RF (wired EOL does not apply).
inline bool isZoneRF(uint8_t zoneId) {
  if (zoneId >= ZONE_COUNT) return false;
  return zoneSrc[zoneId].type == SRC_RF;
}