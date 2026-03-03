#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "statments.h"

struct ZoneBroadcast {
  uint8_t zone;
  uint8_t state;     // 0/1 or your enum encoded
  uint32_t ts_ms;
};

// Queue handle
extern QueueHandle_t qZoneBroadcast;

// Cache: latest state per zone (router maintains)
struct ZoneCacheItem {
  uint8_t state;
  uint32_t ts_ms;
  bool dirty;        // needs publish
};

extern ZoneCacheItem gZoneCache[ZONE_COUNT];

// Non-blocking push (drop if full)
inline bool broadcastPush(const ZoneBroadcast& z) {
  return (xQueueSend(qZoneBroadcast, &z, 0) == pdPASS);
}

void setupZoneBroadcasting();

void startBroadcastTasks();