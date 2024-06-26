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

extern AsyncWebSocket ws;

void notifyClients_pageZones();

void notifyClients_pageUser();

void notifyClients_pageSys();

void notifyClients_rfidRx(const char* data,uint32_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);

void onEvent(AsyncWebSocket *server,AsyncWebSocketClient *client,AwsEventType type,void *arg, uint8_t *data,
size_t len);

#endif