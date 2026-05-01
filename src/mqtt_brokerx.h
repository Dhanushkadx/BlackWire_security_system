#ifndef _MQTT_BROKER_H
#define _MQTT_BROKER_H

#define MQTT_SECURE
#define _DEBUG
#define MQTT_FIRMWARE_VERSION "1.0.0"
#define MQTT_HARDWARE_VERSION "1.0.0"

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#include "typex.h"
#include "statments.h"
#include "alarm.h"
#include "gsm_broker.h"
#include "config_manager.h"
#include "pixel_blink_module.h"
#include "OTA.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

extern "C" {
  typedef void (*_callbackFunctionType7)(void);
}

extern xQueueHandle xQueue_mqtt_Qhdlr;   // MQTT RX -> Alarm Task (strings)
extern bool mqtt_enable;

#ifndef MQTT_QUEUE_TOPIC_MAX
#define MQTT_QUEUE_TOPIC_MAX 24
#endif

// Outbound payload buffer — telemetry, status, rpc responses (compact JSON)
#ifndef MQTT_TX_PAYLOAD_MAX
#define MQTT_TX_PAYLOAD_MAX 512
#endif

// Inbound payload buffer — sized for the largest expected attr/res payload.
// Measured max: zone block attr/res (8 zones, 16-char names) = ~1022 bytes.
// 1200 bytes gives ~178 bytes of headroom above that measured maximum.
#ifndef MQTT_RX_PAYLOAD_MAX
#define MQTT_RX_PAYLOAD_MAX 1200
#endif

typedef struct {
  char topic[MQTT_QUEUE_TOPIC_MAX];
  char payload[MQTT_TX_PAYLOAD_MAX];
  bool retained;
} MqttTxMsg;

extern QueueHandle_t xQueue_mqtt_tx;

void setup_mqtt();
void callback(char *topic, byte *payload, unsigned int length);
void reconnectMQTT();
void mqtt_com_loop();
void setup_subscriptions();

void callback_onMQTT_connection(_callbackFunctionType7 pFn);
void callback_onMQTT_disconnection(_callbackFunctionType7 pFn);

void mqtt_rx_init();
void mqtt_tx_init();

bool mqtt_is_connected();
const char* mqtt_device_id();

void mqtt_publish_telemetry();
void mqtt_publish_latest_attributes();
void mqtt_publish_status(const char* status, const char* reason = nullptr, bool retained = true);
void mqtt_publish_zone_event(uint8_t zone, bool isOpen);
void mqtt_publish_alarm_event(const char* eventName, int zone, const char* reason, const char* alarmType = nullptr);
void mqtt_publish_state_action_event(const char* eventName, uint8_t userId, eInvoking_source source);
void mqtt_publish_power_event(bool powerOk);
void mqtt_publish_sms_received_event(const char* message, const char* number);
void mqtt_publish_boot_snapshot();
void publish_incomming_sms_to_mqtt(char* local_smsbuffer, char* n);

// Load /cfgIndex.json — call during boot after SPIFFS is mounted.
void mqtt_load_cfg_index();

// Clear local version numbers for all zone attr keys (z_atr00_07 … z_atr40_47) and
// rewrite /cfgIndex.json so MQTT re-fetches zone names on next connect.
// Call when zones.bin was reinitialized with defaults (gZoneManager.wasReinitialized()).
void mqtt_invalidate_zone_cfg_versions();

#endif
