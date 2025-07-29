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

// wifi
// the Wifi radio's status
extern int status;
extern bool wifiStarted;
extern bool  setup_web_server_started;

#define TOTAL_PHONE_NUMBER_COUNT 8
extern TimerSW Timer_WIFIreconnect;

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







#endif //ESP32_WEBSOCKET_H
