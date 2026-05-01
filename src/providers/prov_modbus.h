#pragma once
#include <Arduino.h>
#include "../typex.h"

/**
 * ProvModbus — stub Modbus RTU zone provider.
 *
 * Current board has no RS485 port. begin() initialises g_modbusCardEnabled[]
 * from systemConfig so isZoneActive() works correctly for enabled cards.
 * poll() is a no-op; real implementation reads coils from each slave.
 *
 * New board: replace poll() body with actual Modbus RTU reads.
 */
class ProvModbus {
public:
  void begin(const systemConfigTypedef_struct& cfg);
  void poll();  // no-op stub — implement when RS485 hardware is ready
};

extern ProvModbus provModbus;
