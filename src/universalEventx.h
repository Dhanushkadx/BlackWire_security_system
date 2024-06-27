
#ifndef _UNI_EVENTX_H
#define _UNI_EVENTX_H

#include <ArduinoJson.h>
#include "WiFi.h"
#include "typex.h"
#include "statments.h"
#include "alarm.h"
#include "ESP32Time.h"
#include "gsm_broker.h"
#include "config_manager.h"


#ifdef MQTT_OK
#include "mqtt_broker.h"
#endif

byte universal_event_hadler(const char* smsbuffer, eInvoking_source Invoker, uint8_t user_id);


#endif