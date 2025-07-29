#ifndef _MQTT_BROKER_H
#define _MQTT_BROKER_H
#define MQTT_SECURE
//#define _DEBUG
#include "Arduino.h"
#include "typex.h"
#include "statments.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include "alarm.h"
#include "gsm_broker.h"
#include "config_manager.h"
#include "pixel_blink_module.h"

extern "C" {
    typedef void (*_callbackFunctionType7)(void);
}
extern xQueueHandle xQueue_mqtt_Qhdlr;
extern uint8_t zone;
extern bool mqtt_enable;
extern bool state;
void setup_mqtt();
void callback(char *topic, byte *payload, unsigned int length);
void reconnectMQTT();
void send_sensor_state_update_to_mqtt(uint8_t _zone,bool _state);
void publish_json_to_mqtt(const char* jsonStr);
void mqtt_com_loop();
void setup_subscriptions();
void publish_system_state(const char* state, const char* subtopic,bool retaind_flag);
void set_onMQTT_connection(_callbackFunctionType7 pFn);
void transfer_mqtt_data(const char* msg);
void send_rfid_state_update_to_mqtt(const char* rfid);
void publish_incomming_sms_to_mqtt(char* local_smsbuffer, char* n );
void publish_network_info();
#endif