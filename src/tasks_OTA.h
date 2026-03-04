// tasks_OTA.h
#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>

enum class OtaType : uint8_t { Firmware = 0, Spiffs = 1 };

// You provide this callback to publish MQTT status messages.
// Return true if published/queued successfully.
using OtaMqttPublishCb = bool (*)(const char* topic, const char* payload, bool retain);

namespace TasksOTA {

  // Must be called once after SPIFFS is mounted (SPIFFS.begin) very early in boot.
  // It loads any pending OTA request from /ota_req.bin into RAM.
  void bootLoadPending();

  // Returns true if device should boot in "OTA minimal mode"
  bool isOtaBootMode();

  // Called from MQTT callback in normal mode:
  // 1) stores URL + marks pending in SPIFFS
  // 2) (optional) publishes "restarting_for_ota"
  // 3) restarts ESP
  bool markPendingAndReboot(const char* url,
                            OtaType type,
                            OtaMqttPublishCb pubCb,
                            const char* statusTopic);

  // Start OTA subsystem task (queue + task). Call after WiFi is up.
  bool begin(OtaMqttPublishCb pubCb,
             const char* statusTopic,
             uint32_t stackBytes = 12288,
             UBaseType_t priority = (configMAX_PRIORITIES - 1),
             BaseType_t core = 1);

  // In OTA boot mode: call this AFTER MQTT is connected to trigger download.
  // It clears the pending flag first (to avoid reboot loop), then runs OTA.
  bool startFromPending();

  // Optional: query
  bool inProgress();
}