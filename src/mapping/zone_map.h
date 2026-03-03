#pragma once
#include <Arduino.h>
#include "../event_bus.h"

enum SourceType : uint8_t { SRC_NONE=0, SRC_GPIO=1, SRC_ADS=2, SRC_RF=3 };

struct ZoneSource {
  SourceType type;
  uint8_t providerId;   // which GPIO bank / which ADS chip / which RF table
  uint8_t channel;      // GPIO bit index / ADS channel 0..3 / RF entry index
  uint16_t param;       // optional (unused now)
};

// NOTE:
// Edit this table to match your hardware map.
// Placeholder example:
//  - zones 0..3 -> GPIO provider 0, channel 0..3
//  - zones 8..15 -> ADS devices 0 and 1 (4ch each)
//  - others NONE for now
static const ZoneSource zoneSrc[ZONE_COUNT] = {
  {SRC_GPIO, 0, 0, 0},
  {SRC_GPIO, 0, 1, 0},
  {SRC_GPIO, 0, 2, 0},
  {SRC_GPIO, 0, 3, 0},

  {SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},

  {SRC_ADS, 0, 0, 0},{SRC_ADS, 0, 1, 0},{SRC_ADS, 0, 2, 0},{SRC_ADS, 0, 3, 0},
  {SRC_ADS, 1, 0, 0},{SRC_ADS, 1, 1, 0},{SRC_ADS, 1, 2, 0},{SRC_ADS, 1, 3, 0},

  // Remaining zones default NONE
  {SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},
  {SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},
  {SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},
  {SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},
  {SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},
  {SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},
  {SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},
  {SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},
  {SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},
  {SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},
  {SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},
  {SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},{SRC_NONE,0,0,0},
};
