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

// wifi
// the Wifi radio's status
extern int status;
extern bool wifiStarted;
extern bool  setup_web_server_started;

#define TOTAL_PHONE_NUMBER_COUNT 8
extern TimerSW Timer_WIFIreconnect;

void setup_web_server_with_AP();
void setup_web_server_with_STA();
void cleanClients();
void initWebSocket();
void reconnect();
void initSPIFFS();
void initWiFi_AP();
void initWiFi_STA();

String processor(const String &var);
void onRootRequest(AsyncWebServerRequest *request);
void onGetRequest(AsyncWebServerRequest *request);
void initWebServer();






#endif //ESP32_WEBSOCKET_H
