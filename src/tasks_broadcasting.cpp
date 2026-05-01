#include "tasks_broadcasting.h"
#include "ws_tx_queue.h"
#ifdef MQTT_OK
#include "mqtt_brokerx.h"
#endif

QueueHandle_t qZoneBroadcast = nullptr;
ZoneCacheItem gZoneCache[ZONE_COUNT] = {};

void TaskBroadcastRouter(void *parameter)
{
  (void)parameter;

  ZoneBroadcast bz;

  for (;;)
  {
    // Wait for next broadcast event
    if (xQueueReceive(qZoneBroadcast, &bz, portMAX_DELAY) != pdPASS) {
      continue;
    }

    if (bz.zone >= ZONE_COUNT) continue;

    // Update cache
    gZoneCache[bz.zone].state = bz.state;
    gZoneCache[bz.zone].ts_ms = bz.ts_ms;
    gZoneCache[bz.zone].dirty = true;

#ifdef MQTT_OK
    mqtt_publish_zone_event(bz.zone, bz.state != 0);
#endif

    wsTxSendZoneState(bz.zone, bz.state);
  }
}

// ------------------- Task creator -------------------
void startBroadcastTasks()
{
  // Keep your same stack sizes / priorities / core pinning
  xTaskCreatePinnedToCore(TaskBroadcastRouter, "bcast", 3072, nullptr, 2, nullptr, 1);
}

void setupZoneBroadcasting() {
  qZoneBroadcast = xQueueCreate(32, sizeof(ZoneBroadcast));
if (!qZoneBroadcast) Serial.println(F("qZoneBroadcast create failed"));
}
