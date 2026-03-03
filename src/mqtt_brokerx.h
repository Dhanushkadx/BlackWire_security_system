#ifndef _MQTT_BROKER_H
#define _MQTT_BROKER_H

// Keep your existing defines
#define MQTT_SECURE
#define _DEBUG

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

// -----------------------------------------------------------------------------
// Existing globals (RX queue remains the same name to avoid breaking code)
// -----------------------------------------------------------------------------
extern xQueueHandle xQueue_mqtt_Qhdlr;   // MQTT RX -> Alarm Task (strings)
extern bool mqtt_enable;

// -----------------------------------------------------------------------------
// NEW: MQTT TX queue (publish requests)
// -----------------------------------------------------------------------------
#ifndef MQTT_TX_SUBTOPIC_MAX
#define MQTT_TX_SUBTOPIC_MAX 50
#endif

#ifndef MQTT_TX_PAYLOAD_MAX
#define MQTT_TX_PAYLOAD_MAX 128
#endif

typedef struct {
  char subtopic[MQTT_TX_SUBTOPIC_MAX];   // e.g. "info/mode", "sensors/z12"
  char payload[MQTT_TX_PAYLOAD_MAX];     // small payload (text or small json)
  bool retained;
} MqttTxMsg;

extern QueueHandle_t xQueue_mqtt_tx;

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------
void setup_mqtt();
void callback(char *topic, byte *payload, unsigned int length);
void reconnectMQTT();
void mqtt_com_loop();
void setup_subscriptions();

void send_sensor_state_update_to_mqtt(uint8_t _zone, bool _state);
void send_rfid_state_update_to_mqtt(const char* rfid);
void publish_incomming_sms_to_mqtt(char* local_smsbuffer, char* n);
void publish_json_to_mqtt(const char* jsonStr);

// Publish helper (now non-blocking via TX queue)
void publish_system_state(const char* state, const char* subtopic, bool retaind_flag);

// Inbound commands: callback uses this to pass decoded commands to Alarm Task
void transfer_mqtt_data(const char* msg);

void callback_onMQTT_connection(_callbackFunctionType7 pFn);
void callback_onMQTT_disconnection(_callbackFunctionType7 pFn);

// -----------------------------------------------------------------------------
// NEW: RX enqueue helpers
// -----------------------------------------------------------------------------
void mqtt_rx_init();

// -----------------------------------------------------------------------------
// NEW: TX enqueue helpers
// -----------------------------------------------------------------------------
void mqtt_tx_init(); // create TX queue + start TX task (safe to call multiple times)
bool mqtt_tx_enqueue(const char* subtopic, const char* payload, bool retained);
bool mqtt_tx_enqueue_zone(uint8_t zone, bool state, bool retained = true);

#endif
