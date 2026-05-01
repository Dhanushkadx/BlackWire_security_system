#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>   // AsyncWebSocket

// Message types sent over WS TX queue
enum WsEvtType : uint8_t {
  WS_EVT_OK = 1,
  WS_EVT_ERR,
  WS_EVT_DATAX,
  WS_EVT_SCAN_CODE,
  WS_EVT_LOG,
  WS_EVT_ZONE_STATE   // real-time single-zone status push
};


// A compact message passed between tasks (no heap allocations)
struct WsMsg {
  WsEvtType type;

  char page[12];      // "zones" / "remotes" / "sys" / etc.
  char message[96];   // message or payload string (code string can go here)

  uint32_t v0;        // optional numeric fields (slot/user/zone/etc.)
  uint32_t v1;
  uint32_t ts;        // millis() snapshot
};

// ---- API ----

// Must be called once after your global AsyncWebSocket ws is constructed
// Example: wsTxAttach(&ws);
void wsTxAttach(AsyncWebSocket* ws);

// Create queue + start task (safe to call once)
bool wsTxBegin(uint16_t queueDepth = 16, uint16_t taskStackWords = 4096, UBaseType_t taskPrio = 3);

// Optional: clear queued messages
void wsTxClear();

// Enqueue helpers (thread-safe)
bool wsTxSendOk(const char* page, const char* msg);
bool wsTxSendErr(const char* page, const char* msg);

// Generic data with optional fields (message can be empty)
bool wsTxSendData(const char* page, const char* msg, uint32_t v0 = 0, uint32_t v1 = 0);

// Scan result helper (fits your portal: respHeader:"data", scan_rfid:"...")
// slotOrIndex is optional (slot for remotes, zone index for zones scan)
bool wsTxSendScan(const char* page, const char* codeStr, uint32_t slotOrIndex = 0);

bool wsTxLog(const char* src, const char* msg);

// Push a single zone state update to all WS clients.
// status: 0=CLOSE 1=OPEN 2=FAULT 3=UNAVAILABLE
bool wsTxSendZoneState(uint8_t zone, uint8_t status);