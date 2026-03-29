#include "tasks_zone_engine.h"

#include "event_bus.h"                   // rawPop(), RawInputEvent
#include "zone_engine.h"                 // ZoneEngine
#include "providers/prov_gpio_readable.h"
#include "providers/prov_ads1115_readable.h"

// If you also poll RF provider here, include it and add it to pollTask.
// (In your current code, RF is handled via interrupts/listener elsewhere.)

// ------------------- Extern objects from main -------------------
extern ZoneEngine zoneEngine;
extern ProvGPIO provGpio;
extern ProvADS1115 provAds;

// ------------------- pollTask -------------------
void pollTask(void *parameter)
{
  (void)parameter;

  for (;;)
  {
    provGpio.poll();
    provAds.poll();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// ------------------- zoneEngineTask -------------------
void zoneEngineTask(void *parameter)
{
  (void)parameter;

  RawInputEvent e;
  for (;;)
  {
    if (rawPop(e, portMAX_DELAY))
    {
      Serial.println(F("Raw event popped"));
      zoneEngine.onRaw(e);
    }
  }
}

// ------------------- zoneTickTask -------------------
void zoneTickTask(void *parameter)
{
  (void)parameter;

  for (;;)
  {
    zoneEngine.tick(millis());
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ------------------- Task creator -------------------
void startZoneEngineTasks()
{
  xTaskCreatePinnedToCore(pollTask,       "poll", 3072, nullptr, 3, nullptr, 1);
  xTaskCreatePinnedToCore(zoneEngineTask, "zone", 3072, nullptr, 4, nullptr, 1);
  xTaskCreatePinnedToCore(zoneTickTask,   "tick", 3072, nullptr, 2, nullptr, 1);
}
