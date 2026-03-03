#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "statments.h"

// ---------- Raw input events (from providers -> zone engine) ----------
enum RawFlags : uint8_t {
  RAWF_NONE   = 0,
  RAWF_FAULT  = 1 << 0,
  RAWF_TAMPER = 1 << 1,
  RAWF_RF     = 1 << 2,
  RAWF_ANALOG = 1 << 3,
};

struct RawInputEvent {
  uint8_t zone;     // 0..47
  uint8_t level;    // 0/1
  uint8_t flags;    // RawFlags
  uint32_t t_ms;    // millis()
};

// ---------- Zone events (from zone engine -> system/portal/gsm/etc) ----------
enum ZoneState : uint8_t {
  ZS_CLOSE = 0,
  ZS_OPEN  = 1,
  ZS_FAULT = 2,
};

struct ZoneEvent {
  uint8_t zone;
  ZoneState state;
  uint32_t t_ms;
};

// ---------- API ----------
bool eventBusInit(uint16_t rawDepth = 64, uint16_t zoneDepth = 64);

bool rawPush(const RawInputEvent& e);
bool rawPop(RawInputEvent& e, TickType_t waitTicks = portMAX_DELAY);

bool zonePush(const ZoneEvent& e);
bool zonePop(ZoneEvent& e, TickType_t waitTicks = portMAX_DELAY);
