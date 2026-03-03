#ifndef UTILITY_H
#define UTILITY_H

#include <Arduino.h>
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
