// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------
#ifndef _SOCKET_H
#define _SOCKET_H

#include "Arduino.h"
#include "statments.h"
#include "typex.h"
#include "ESPAsyncWebServer.h"
#include "ArduinoJson.h"
#include "FS.h"
#include "WiFi.h"
#include <SPIFFS.h>
#include "call_backs.h"
#include "sensor_scan.h"
#include "Adafruit_FONA.h"

extern AsyncWebSocket ws;

static void wsSendErr(AsyncWebSocketClient* c, const char* page, const char* msg);
static void wsSendOk(AsyncWebSocketClient* c, const char* page, const char* msg);
void sendPageSys(AsyncWebSocketClient* c);
void sendPageZones(AsyncWebSocketClient* c);
void sendPageInfo(AsyncWebSocketClient* c);
void sendPageContacts(AsyncWebSocketClient* c);
static bool saveSystemSettingsFromReq(JsonDocument& req, const char** errMsgOut);
void handleWebSocketMessage(AsyncWebSocketClient* client, void *arg, uint8_t *data, size_t len);
static bool writeJsonAtomic(const char* path, JsonDocument& doc);


void onEvent(AsyncWebSocket *server,AsyncWebSocketClient *client,AwsEventType type,void *arg, uint8_t *data,
size_t len);

#endif