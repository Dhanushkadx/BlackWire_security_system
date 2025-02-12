

#ifndef _GSM_BROKER_H
#define _GSM_BROKER_H

#include "Arduino.h"
#include "statments.h"
#include "typex.h"
#include "call_backs.h"
#include "Adafruit_FONA.h"
#include "ESP32Time.h"
#include "msg_store.h"

//#define GSM_MODULE_AUTH
extern bool gsm_init_done;
extern bool gsm_available;
extern HardwareSerial *fonaSerial;
extern bool request_from_sim800;
extern uint8_t sms_broadcast_index;
#define SMS_BUFFER_LENGTH 250
extern uint8_t sms_buffer_msg_count;
extern uint8_t Current_caller_state;
extern uint8_t alarm_calling_index;
extern bool thisIs_Restart;
extern bool timesync_need;
bool gsm_init();
	 
void creatSMS(const char* buffer,uint8_t type, const char* number);// creat a SMS

void creatSMS_LL(const char* buffer,uint8_t type, const char* number);

void ultimate_sms_hadlr();
					

bool check_SMS_TASK_suspend_request();
boolean phone_number_validat(const char* num_char);

boolean phone_number_correction(char *num_char);

void flushSerial();

bool ultimate_gsm_listiner();
void gsm_manager();

		
void incoming_sms_process(char* local_smsbuffer, char* n );

void creat_arm_sms(char* str_invorker);

void creat_disarm_sms(char* str);

void creat_panic_sms(char* str);

void creat_power_sms(bool power_state);

//retuns
//0 - succesfully done
//1 - somethong is wrong
//cases
//0.check network
//1. init call
//2. check call success if not retry case
///3. check answer or busy
//4.play musics
//5.check hang up
//6.move to next number
uint8_t ultimate_call_hadlr();



bool waitForMutex_GSM();


void tell_lcd(char *number);

void setTime_from_gsm();
void addTimeStamp(char* sms_buffer);


void convertTime(char* input_string, char* output_string);

void setESP32_rtc(char *timeChars);

void releaseMutex_GSM();

#endif