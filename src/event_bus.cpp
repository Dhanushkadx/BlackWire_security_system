#include "event_bus.h"

static QueueHandle_t qRaw  = nullptr;
static QueueHandle_t qZone = nullptr;

bool eventBusInit(uint16_t rawDepth, uint16_t zoneDepth) {
  if (!qRaw)  qRaw  = xQueueCreate(rawDepth,  sizeof(RawInputEvent));
  if (!qZone) qZone = xQueueCreate(zoneDepth, sizeof(ZoneEvent));
  return qRaw && qZone;
}

bool rawPush(const RawInputEvent& e) {
  if (!qRaw) return false;
  Serial.println("Pushing raw event");
  return xQueueSend(qRaw, &e, 0) == pdTRUE;
}

bool rawPop(RawInputEvent& e, TickType_t waitTicks) {
  if (!qRaw) return false;
  return xQueueReceive(qRaw, &e, waitTicks) == pdTRUE;
}

bool zonePush(const ZoneEvent& e) {
  if (!qZone) return false;
  return xQueueSend(qZone, &e, 0) == pdTRUE;
}

bool zonePop(ZoneEvent& e, TickType_t waitTicks) {
  if (!qZone) return false;
  return xQueueReceive(qZone, &e, waitTicks) == pdTRUE;
}
