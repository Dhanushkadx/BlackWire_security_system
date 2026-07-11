// tasks_OTA.h
#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>

// ── OTA outcome ────────────────────────────────────────────────────────────
// OTA_OK is reported after a reboot (the run itself never returns on success,
// it reboots); any other value is a failure already reported on the status
// topic.
enum class OtaResult : uint8_t {
  OK = 0,
  PAUSED,
  ERR_META,
  ERR_BEGIN,
  ERR_TIMEOUT,
  ERR_WRITE,
  ERR_SHA,
  ERR_END,
  ERR_FS_SIZE,
  ERR_TARGET,
};

// You provide this callback to publish MQTT status messages.
// Return true if published/queued successfully.
using OtaMqttPublishCb = bool (*)(const char* topic, const char* payload, bool retain);

namespace TasksOTA {

  // Start the OTA subsystem task (queue + task). Call once after MQTT is set up
  // (mirrors the existing call site in reconnectMQTT()).
  bool begin(OtaMqttPublishCb pubCb,
             const char* statusTopic,
             uint32_t stackBytes = 12288,
             UBaseType_t priority = 1,
             BaseType_t core = 0);

  // Feed EVERY inbound MQTT message here from callback() BEFORE the normal
  // per-topic dispatch. During an active OTA it consumes the raw-binary
  // chunk/res and the JSON meta/res replies and returns true; otherwise
  // returns false (always false when no OTA is running), so it's safe to
  // call unconditionally for every message.
  bool consume(const char* topic, const uint8_t* payload, unsigned int len);

  // Trigger a fresh OTA (call from the MQTT cmd/sys/ota/firmware|spiffs
  // handlers, which run inside client.loop()). Only latches the request —
  // service() starts the actual session on the next MQTT-task iteration.
  // Returns false if an OTA is already active/paused or one is already latched.
  bool request(bool is_fs);

  // Run pending OTA work on the MQTT task. Call once per mqtt_com_loop()
  // iteration, AFTER client.loop(). PubSubClient is single-threaded, so the
  // whole OTA must run here, never on a side task.
  void service();

  // Call from reconnectMQTT() after a successful (re)connect: resumes a
  // paused OTA (re-subscribes, re-does the meta handshake, continues the
  // chunk pull from the saved offset) if one is pending.
  bool resumeIfPaused();

  // True while an OTA is paused waiting for the MQTT link to recover.
  bool pending();

  // True while an OTA session exists (running OR paused).
  bool active();

  // Alias for active() kept for API continuity.
  bool inProgress();

  // Call once, early in setup(): latches the NVS boot-outcome markers left by
  // a previous OTA (success / interrupted) so bootReportIfNeeded() can report
  // them once MQTT is back up.
  void bootCheck();

  // Call right after a successful MQTT (re)connect: publishes UPDATED /
  // FAILED "aborted" for the previous boot's OTA outcome, if any. No-op if
  // there was nothing to report.
  void bootReportIfNeeded();

}
