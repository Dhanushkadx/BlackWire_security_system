#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// IO task(s): buzzer / relay / pixel blinking
void Task9code(void *parameter);

// Convenience: create the IO task with your existing stack/prio/core choice.
void startIoTasks();
