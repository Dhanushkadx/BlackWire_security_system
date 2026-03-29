#ifndef UTILITY_H
#define UTILITY_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Debug logging helpers (only active when _DEBUG is defined)
// Prints a formatted uptime timestamp: [HH:MM:SS.mmm]
// ---------------------------------------------------------------------------
#ifdef _DEBUG
inline void debug_print_timestamp() {
  const uint32_t ms      = millis();
  const uint32_t seconds = ms / 1000;
  const uint32_t minutes = seconds / 60;
  const uint32_t hours   = minutes / 60;
  Serial.printf("[%02lu:%02lu:%02lu.%03lu] ",
                (unsigned long)(hours   % 24),
                (unsigned long)(minutes % 60),
                (unsigned long)(seconds % 60),
                (unsigned long)(ms      % 1000));
}
// Prints a divider line to visually separate log blocks
inline void debug_print_divider() {
  Serial.println(F("─────────────────────────────────────────────────────"));
}
#endif
#include <WiFi.h>
#include <ArduinoJson.h>
#include "gsm_broker.h"
#include "mqtt_brokerx.h"  // for mqtt.publish(), use your transport layer instance

// Constants
#define NUM_ZONES 48

// Globals (or you can make them private inside cpp)
extern uint8_t zoneState[6];
extern bool systemArmed;

// Functions
void setZone(uint8_t zone, bool active);
void zonesToHex(char* output);
void publish_system_startup_msg();
void publish_network_info();
void publish_health_info(float batteryVoltage, uint32_t restartCount);

#endif
