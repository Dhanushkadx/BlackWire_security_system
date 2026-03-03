#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Zone engine / providers tasks
// - pollTask: polls providers (GPIO + ADS) and pushes raw events
// - zoneEngineTask: consumes raw events and feeds ZoneEngine
// - zoneTickTask: periodic debounce/tick processing

void pollTask(void *parameter);
void zoneEngineTask(void *parameter);
void zoneTickTask(void *parameter);

// Convenience: create the tasks with your existing stack/prio/core choices.
void startZoneEngineTasks();
