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
extern TimerSW Timer_gsmled;
extern EventGroupHandle_t EventRTOS_buzzer;
extern EventGroupHandle_t EventRTOS_siren;
extern EventGroupHandle_t EventRTOS_gsmled;

void setup_gsmled();
void setup_siren();
void setup_buzzer();
void gsm_led_update();

void relayTask();
void buzzer();
void disarm_busy_buzzer();
void arm_busy_buzzer();
void alarm_bell_time_out();
void gsm_led_blink(uint8_t count,uint16_t onTime, uint16_t offTime, uint32_t interval);
#endif