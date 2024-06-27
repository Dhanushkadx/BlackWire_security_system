#ifndef _SIREN_H
#define _SIREN_H

#include "Arduino.h"
#include "statments.h"
#include "pinsx.h"
#include "typex.h"
#include "TimerSW.h"
#include "call_backs.h"

extern TimerSW Timer_siren;
extern TimerSW Timer_buzzer;
extern EventGroupHandle_t EventRTOS_buzzer;
extern EventGroupHandle_t EventRTOS_siren;

void relayTask();
void buzzer();
void disarm_busy_buzzer();
void arm_busy_buzzer();
void alarm_bell_time_out();
#endif