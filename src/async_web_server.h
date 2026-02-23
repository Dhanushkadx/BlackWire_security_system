// ESP32_websocket.h

#ifndef ESP32_WEBSOCKET_H
#define ESP32_WEBSOCKET_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "AsyncWebSocket.h"
#include <ArduinoJson.h>
#include "statments.h"
#include "typex.h"
#include "TimerSW.h"
#include "socket_function.h"
#include <AsyncElegantOTA.h>
#include "pixel_blink_module.h"
// ZoneStorage module
#include "ZoneStorage.h"

// wifi
// the Wifi radio's status
extern int status;
extern bool wifiStarted;
extern bool  setup_web_server_started;

#define TOTAL_PHONE_NUMBER_COUNT 8
extern TimerSW Timer_WIFIreconnect;

// --------- helper: build keys safely ----------
static inline void makeKey(char* out, size_t outSz, int idx, const char* suffix);
// --------- helper: set/clear bit ----------
static inline void setBit(uint8_t &v, uint8_t bit, bool en);

void setup_web_server_with_AP();
void setup_web_server_with_STA();
void setup_web_server_with_STA_info();
void cleanClients();
void initWebSocket();
void initSPIFFS();
void initWiFi_AP();
void initWiFi_STA();

String processor(const String &var);
void onRootRequest(AsyncWebServerRequest *request);
void onGetRequest(AsyncWebServerRequest *request);
void initWebServer();

void onRootRequest_info(AsyncWebServerRequest *request);

void initWebServer_info();



void onGetRequest(AsyncWebServerRequest *request);
static bool handleZonesSubmit(AsyncWebServerRequest *request);
static bool handlePhonesSubmit(AsyncWebServerRequest *request);
static bool handleConfigSubmit(AsyncWebServerRequest *request);



#endif //ESP32_WEBSOCKET_H
