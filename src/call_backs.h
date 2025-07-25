
#ifndef CALL_BACKS_H
#define CALL_BACKS_H

#include "Arduino.h"
#include "typex.h"
#include "pinsx.h"
#include "statments.h"
#include "ArduinoJson.h"
#include "gsm_broker.h"
#include "FS.h"
#include <SPIFFS.h>
#include "rf_methods.h"
#include "mqtt_broker.h"
#include "msgRingBuffer.h"

//#include "message_buffer.h"
#define SMS_STRCUT_MAX_MGS 10

//extern NEW_SMS SMS_to_be_sent_FIXDMEM[];
uint8_t call_back_alarm_Calling();

bool call_back_creat_alarm_sms(const char* buffer_sms, int x);

uint8_t call_back_sms_loop_status_check();
void call_back_alarm_snoozed();
uint8_t call_back_alarm_bell_time_out();
void call_back_DISARM(uint8_t user, const char* _msg, eInvoking_source _last_invoker);

void call_back_ARM(uint8_t user, const char* _msg, eInvoking_source _last_invoker);

void call_back_system_is_not_ready();
	

void call_back_system_is_ready();
bool call_back_arm_faild(const char* srt ,int index);

void call_back_arm_can_not_be_done();


void call_back_Exit_delay_timer_start();

void call_back_Entry_delay_timer_start();
void call_back_zone_state_update_notify(uint8_t zone0_7, uint8_t zone8_15, uint8_t zone16_23, uint8_t zone24_31, uint8_t zone32_39, uint8_t zone40_47);

// chime alarm using i2c . zone state update on CAN bus will be sent separately. see >call_back_zone_state_update_notify();
bool call_back_zone_status_update(const char* srt ,int index);

bool call_back_Entry_delay_time_out(const char* srt ,int index);

bool call_back_Exit_delay_time_out(const char* srt ,int index);
void call_back_alarm_notify(uint8_t alarm_zone);
char* get_device_name(uint8_t device_index);
void set_device_name(uint8_t device_index, const char* device_name);
byte set_zone_param(const char* smsbuffer);
int8_t comp_device_RFID(const char* rf_id_rx);
char* get_device_RFID(uint8_t device_index);
void set_device_RFID(uint8_t device_index, const char* rf_id);
int8_t comp_User_id(const char *number);
int8_t comp_remote_RFID(uint32_t rx_rf_id_uint, uint8_t command_length);
char* get_remote_RFID(uint8_t device_index);
void set_remote_RFID(uint8_t device_index, const char* rf_id);
char* get_GSM_number(uint8_t gsm_number_index);

void set_GSM_number(uint8_t gsm_number_index, const char* gsm_number);

bool get_is_GSM_number_call(uint8_t gsm_number_index);
void set_GSM_number_is_call(uint8_t gsm_number_index, bool call_en);
bool get_is_GSM_number_sms(uint8_t gsm_number_index);

void set_GSM_number_security_level(uint8_t gsm_number_index, uint8_t security_level);
uint8_t get_GSM_number_security_level(uint8_t gsm_number_index);
void set_GSM_number_is_sms(uint8_t gsm_number_index, bool sms_en);
void get_EVENT_infor(uint8_t event_index, char*event_buffer, uint8_t len);
void set_EVENT_infor(uint8_t event_index, char* discription);
void save_event_info(uint8_t event_type_id, uint8_t user_id, char* event_time, char* event_discriptio);
void call_back_chime_sound();
void setup_call_backs();

void onMqtt_connection();

#endif
