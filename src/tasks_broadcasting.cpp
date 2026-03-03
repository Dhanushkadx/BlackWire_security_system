#include "tasks_broadcasting.h"

// ------------------- Extern globals from main -------------------
#ifdef MQTT_OK
extern xQueueHandle xQueue_mqtt_Qhdlr;
// If your MQTT TX task already reads a queue, we can push zone IDs into it.
// Replace this with YOUR existing MQTT TX queue type:
//extern QueueHandle_t qMqttTx;   // <-- you already have something like this
#endif

QueueHandle_t qZoneBroadcast = nullptr;
ZoneCacheItem gZoneCache[ZONE_COUNT] = {};

// ------------------- Task1: Broadcasting zone events-------------------

// Example: send just a zone id to mqtt tx task
static inline void mqttNotifyZoneDirty(uint8_t zone)
{
  // Non-blocking: if full, MQTT task can still publish later via full snapshot
  xQueueSend(xQueue_mqtt_Qhdlr, &zone, 0);
}

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

    // Fanout: notify MQTT task (and later CAN/Modbus tasks the same way)
    mqttNotifyZoneDirty(bz.zone);

    // TODO later:
    // canNotifyZoneDirty(bz.zone);
    // modbusNotifyZoneDirty(bz.zone);
    // websocketNotifyZoneDirty(bz.zone);
  }
}

// ------------------- Task creator -------------------
void startBroadcastTasks()
{
  // Keep your same stack sizes / priorities / core pinning
  xTaskCreatePinnedToCore(TaskBroadcastRouter, "bcast", 4096, nullptr, 2, nullptr, 1);
}

void setupZoneBroadcasting() {
  qZoneBroadcast = xQueueCreate(64, sizeof(ZoneBroadcast)); // 64 is usually enough
if (!qZoneBroadcast) Serial.println(F("qZoneBroadcast create failed"));
}
