

#ifndef _GSM_BROKER_H
#define _GSM_BROKER_H

#include "Arduino.h"
#include "statments.h"
#include "typex.h"
#include "call_backs.h"
#include "Adafruit_FONA.h"
#include "ESP32Time.h"
#include "msg_store.h"
#include <ArduinoJson.h>
#include "pixel_blink_module.h"
#include "siren.h"
#include "pinsx.h"


#define SIM800_OK 0
#define NO_SIM 1
#define NO_RESP 2
#define AUTH_FAILD 3

#define ESP_TXD GSM_RX
#define ESP_RXD GSM_TX
#define ESP_RST RESET_PIN
extern PixelBlink pixel;

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
uint8_t gsm_init();
	 
void creatSMS(const char* buffer,uint8_t type, const char* number);// creat a SMS

//void creatSMS_LL(const char* buffer,uint8_t type, const char* number);


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

uint8_t getSignal_strength();

extern uint8_t gsmsignal_rssi;
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

bool setTime_from_gsm();
void addTimeStamp(char* sms_buffer);


void convertTime(char* input_string, char* output_string);

void setESP32_rtc(char *timeChars);

void releaseMutex_GSM();

#endif