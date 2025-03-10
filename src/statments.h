/*
 * statments.h
 *
 * Created: 24/10/2019 09:54:54
 *  Author: dhanushkac
 */ 

/*
enumerator eRunning?
enumerator eReady?
enumerator eBlocked?
enumerator eSuspended?
enumerator eDeleted?
enumerator eInvalid?*/

#ifndef STATMENTS_H_
#define STATMENTS_H_
#include "Arduino.h"
#include "typex.h"
#include "WiFi.h"
#include "TimerSW.h"
#include "alarm.h"
#include <freertos/message_buffer.h>
#include "ESP32Time.h"
#include "ESPAsyncWebServer.h"
#define GSM_OK
#define MQTT_OK
extern AsyncWebSocket ws;
extern ESP32Time rtc;
extern ALARM myAlarm_pannel;
extern EventGroupHandle_t EventRTOS_lcd;
extern EventGroupHandle_t EventRTOS_lcdkeyPad;
extern EventGroupHandle_t EventRTOS_gsm;
extern EventGroupHandle_t EventRTOS_buzzer;
extern EventGroupHandle_t EventRTOS_siren;

extern SemaphoreHandle_t xBinarySemaphore;
extern SemaphoreHandle_t xMutex_GSM;
extern SemaphoreHandle_t xMutex_GSM_CALLING;
extern SemaphoreHandle_t xMutex_I2C;


/* this variable hold queue handle */
extern xQueueHandle xQueue;
/* this variable hold queue handle */
extern xQueueHandle xQueue_sensor_state;

extern size_t xBufferSizeBytes_number;
extern MessageBufferHandle_t xMessageBuffer_number;
extern MessageBufferHandle_t xMessageBuffer_zone;
extern size_t xBufferSizeBytes_zone;

extern TimerSW Timer_call_init_delay;
extern TimerSW Timer_call_answer_delay;
extern TimerSW Timer_sms_send_delay;
extern TimerSW Timer_sms_netwok_busy_delay;
extern TimerSW Timer_sms_read_interal;
extern TimerSW Timer_sys_update;
extern TimerSW Timer_keypad_lcd_event;
extern TimerSW Timer_battery_charge;
extern TimerSW Timer_alarm_calling_time_out;

extern TaskHandle_t Task1;
extern TaskHandle_t Task2_sms;
extern TaskHandle_t Task3;
extern TaskHandle_t Task4;
extern TaskHandle_t Task5;
extern TaskHandle_t Task6;
extern TaskHandle_t Task7;
extern TaskHandle_t Task8;
extern TaskHandle_t Task9;
extern TaskHandle_t Task10;

extern void Task1code( void * parameter );
extern void Task2code_sms( void * parameter );
extern void Task3code_lcd( void * parameter );
extern void Task4code_gsm_ctrl( void * parameter );
extern void Task5code_call( void * parameter );
extern void Task6code(void * parameter);
extern void Task7code( void * parameter );
extern void Task8code( void * parameter );
extern void Task9code( void * parameter );
extern void Task10code( void * parameter );

extern void get_eInvoker_type_to_char(eInvoking_source invoker, char* buffer);
extern byte universal_event_hadler(const char* smsbuffer, eInvoking_source Invoker, uint8_t user_id);//0- gsm 1-lcd
extern void eeprom_save();
extern void eeprom_load(uint8_t mode);
extern uint8_t initiliz_sensor_data();

extern eMain_state eCurrent_state;
extern eMain_state ePrev_state;
/* define event bits */
#define TASK_1_BIT  ( 1 << 0 ) //1
#define TASK_2_BIT  ( 1 << 1 ) //10
#define TASK_3_BIT  ( 1 << 2 ) //100
#define TASK_4_BIT  ( 1 << 3 ) //1000
#define TASK_5_BIT  ( 1 << 4 ) //10000
#define TASK_6_BIT  ( 1 << 5 ) //100000
#define TASK_7_BIT  ( 1 << 6 ) //100000
#define ALL_SYNC_BITS (TASK_1_BIT | TASK_2_BIT | TASK_3_BIT | TASK_4_BIT | TASK_5_BIT | TASK_6_BIT | TASK_7_BIT) //111

#define MAC_ADDR_SIZE 6
//POWER
#define FONA_RST 5

/*char lcd_buffer0[17];
char lcd_buffer1[17];
char lcd_time[17];
#define LCD_ROWS  2
#define LCD_COLS  16*/

#define TOTAL_DEVICES 48
#define TOTAL_PHONE_NUMBER_COUNT 8
#define  EVENT_LOG_MAX 10


#define ADDR_OFFSET_EEPROM_NAME 380
#define ADDR_OFFSET_EEPROM_GSM_NUMBERS 1370
#define ADDR_OFFSET_EEPROM_REMOTES 1520
#define ADDR_OFFSET_EEPROM_EVENTS 1650

#define JSON_DOC_SIZE_DEVICE_DATA 6144
#define JSON_DOC_SIZE_ZONE_DATA 2048
#define JSON_DOC_SIZE_USER_DATA 2048
#define JSON_DOC_SIZE_CONFIG_DATA 1024
extern unsigned long try_time;



extern const uint8_t sensor_pin_count;
extern Struct_GPIO_INFO GPIO_array[];
extern  MY_SENS any_sensor_array[];




extern systemConfigTypedef_struct systemConfig;


extern eSYS_MODE system_mode;




//Invorker IDS
#define PSERIAL0_ID 0
#define PSERIAL3_ID 1
#define GSM_MODULE_ID 2
#define CARDA_ID 3
#define CARDB_ID 4
#define CARDC_ID 5
#define LCDA_ID 6
#define LCDB_ID 7
#define LCDC_ID 8
// sensor arrangement
//0-15 CARD A (8 on board) other 8 is not used on current hardware
//16-31 CARD B
//32- to end CARD C

//SERIAL3
#define NEX_RET_STRING_HEAD     (0x70)
#define NEX_RET_CMD_FINISHED	(0x01)
#define NEX_RET_NUMBER_HEAD     (0x71)
// CAN
#define REQ_ZONE_STATE_UPDATE    0x01
#define REQ_SYS_STATE            0x02
#define REQ_NUM_PARA_255         0x03
#define SEND_ZONE_STATE_UPDATE   0x04
#define SEND_SYS_STATE           0x05
#define SEND_NUM_PARA_255        0x06// reply for request data

#define SET_ZONE_STATE           0x07
#define SET_SYS_STATE            0x08
#define SET_NUM_PARA_255         0x09 // [id] [para id] [value]...
#define ACK_RX					 0x10

#define BRODCAST_SYS_UPDATE    0x11
#define ACK_CAN					 0x22	
//#define BRODCAST_SYS_STATE       0x12


// para ids
#define SYS_STATUS 0
#define ETZ 1 // entry zone
#define XTZ 2 // exit zone
#define ET  3 //  entrance time 
#define XT  4 // exit time
#define BET 5 // bell time
#define FLZ 6 //faulty zones
#define ALM 7 // alarm mode 	USER_SELECT=0, AS_ITIS_NO_BYPASS, AS_ITIS_BYPASS,
#define ALZ 8// alarming zones 
#define ZST 9// zone state
#define ZBS 10// zone bypasse

//sysstate
#define CAN_BIT_MSK_HOME_ARM 1
#define CAN_BIT_MSK_AWAY_ARM 2
#define CAN_BIT_MSK_DISARM 0
#define CAN_BIT_MSK_NOT_READY 3
#define CAN_BIT_MSK_ALARMING 4
#define CAN_BIT_MSK_TIME_OUT 5

#endif /* STATMENTS_H_ */