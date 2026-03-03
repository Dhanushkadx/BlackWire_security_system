#include "tasks_io.h"
#include "pixel_blink_module.h"

// ------------------- Extern objects/functions from main/project -------------------
extern PixelBlink pixel;

extern void buzzer();
extern void relayTask();

// Task handle (defined in main)
extern TaskHandle_t Task9;

// ------------------- Task9: IO loop -------------------
void Task9code(void *parameter)
{
  (void)parameter;

  Serial.print(F("Task9 is running on core "));
  Serial.println(xPortGetCoreID());

  for (;;)
  {
    delay(1);      // keep original behavior
    buzzer();
    relayTask();
    pixel.update();
  }
}

// ------------------- Task creator -------------------
void startIoTasks()
{
  xTaskCreatePinnedToCore(Task9code, "Task9", 3048, nullptr, 1, &Task9, 1);
}
