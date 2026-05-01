#include "prov_modbus.h"

// Definition of the extern declared in zone_map.h.
// All cards disabled by default; begin() populates from config.
bool g_modbusCardEnabled[5] = {false, false, false, false, false};

ProvModbus provModbus;

void ProvModbus::begin(const systemConfigTypedef_struct& cfg) {
  for (uint8_t i = 0; i < 5; i++) {
    g_modbusCardEnabled[i] = cfg.mbCard[i];
    Serial.printf("[MODBUS] Card %u (Z%02u-Z%02u): %s\n",
      i + 1, 8 + i * 8, 15 + i * 8,
      g_modbusCardEnabled[i] ? "ENABLED" : "disabled");
  }
  // TODO (new board): initialise RS485 serial port here
}

void ProvModbus::poll() {
  // TODO (new board): read coil registers from each enabled slave
  // and push RawInputEvent{zoneId, value, RAWF_DIGITAL, millis()} per channel
}
