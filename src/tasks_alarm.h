#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "tasks_broadcasting.h"

// Alarm/command/event task(s)
// - Task1: consumes MQTT commands + ZoneEvent queue, runs alarm watcher, handles serial + RF
// - Task2: SMS handler loop
// - Task4: GSM manager loop

void Task1code(void *parameter);
void Task2code_sms(void *parameter);
void Task4code_gsm_ctrl(void *parameter);
void TaskBroadcastRouter(void *parameter);

// Convenience: create the tasks with your existing stack/prio/core choices.
void startAlarmTasks();
