/*
 * typex.h
 *
 * Created: 6/24/2021 8:37:09 AM
 *  Author: dhanu
 */ 


#ifndef TYPEX_H_
#define TYPEX_H_
//#ifdefined (ARDUINO) && ARDUINO >= 100
#include "arduino.h"

enum eSYS_MODE{CONFIG_MODE, NOMAL_MODE_WIFI, NOMAL_MODE_NO_WIFI};

typedef enum eLCD_state{BEGING_LCD,
	KEY_PAD_INPUT,
	SYSTEM_ARM,
	DISARM,
	ALARMING,
	RF_SCANING,
	PANIC_LCD,
	ENTRY_TIMER_START,
	ARMING_EXIT_TIMER_START,
	ENTRY_TIMER_STOP,
	ARMING_EXIT_TIMER_STOP,
} SYS_STATE;



enum eMain_state{BEGING,SYS1_IDEAL,SYS2_IDEAL,ALARM_CALLING,ALARM_SMS_SENDING,DEACTIVE,PANIC,TRANCITION};
	
enum eInvoking_source{PSERIAL0,GSM_MODULE,RF,LCD,INTERNEL_KEY_PAD,SYSTEM_ITSELF,CARD_A,CARD_B,CARD_C,WEB,APP};

enum eDisplay_state{LOADING,HOME, SETTINGS};

enum eBuzzer_state{BUZZER_DISARM,BUZZER_ARM,BUZZER_OFF,BUZZER_ALARM,BUZZER_GSM_ERROR,BUZZER_RF};

typedef struct systemConfig{
	uint8_t sensor_debounce_time;
	uint8_t entry_delay_time;
	uint8_t exit_delay_time;
	uint8_t cli_access_level;
	char last_sms_sender[15];
	boolean battery_charging_en;
	boolean ac_power;
	bool card_A_en;
	bool card_B_en;
	bool card_C_en;
	bool card_D_en;
	bool card_E_en;
	bool card_F_en;
	uint16_t bell_time_out;
	eMain_state last_system_state;
	char wifissid_ap[25]; //":"dxdxdxdxdx",
	char wifissid_sta[25]; //":"dxdxdxdxdx",
	char wifipass[25]; //": "xxxxxxxxxx",
	bool wifi_sta_en;
	bool mqtt_en;
	uint8_t call_attempts;//": 2,
	boolean call_en;  //": true,
	char  installer_no[25];  //":"000000000000",
	char installer_pass[25];   //":"000000",
	bool gsm_module_enable;
	//boolean power_notify;
	//uint8_t alarm_call_delay;
	//uint8_t event_index;
	
} systemConfigTypedef_struct;

#define DEVICE_NAME_MAX_LENGTH 10// my door
#define TOTAL_DEVICES 48
#define RF_DEVICE_START_INDEX 0
typedef struct sensorAtribute{
	//byte device_index;
	uint8_t device_state;
	//bool exit_delay_en;
	//bool entry_delay_en;
	//boolean sensor_en;
	//boolean sensor_bypased;
	uint8_t device_type;
	uint8_t device_card_id;
	//boolean last_alarm_state;
	//boolean last_state;
	long last_updated_time_stamp;
	
} MY_SENS, *pMY_SENS;


enum device_type_attri{
	BIT_MASK_24H = 0,
	BIT_MASK_RF,
	BIT_MASK_SILENT,
	BIT_MASK_PERIMETER,  
	};
	
enum device_state_attri{
	
	BIT_MASK_EXIT_DELAY = 0,
	BIT_MASK_ENTRY_DELAY,
	BIT_MASK_ENABLE,
	BIT_MASK_BYPASSED,
	BIT_MASK_ALARM,
	BIT_MASK_LAST_STATE,
	BIT_MASK_AVAILABLE
};

typedef struct creat_new_sms {
	String msg_content;
	//char msg_content[159]; 
	//char* msg_content;
	uint8_t type; //group sms-1, privet reply-2, notification phones only-3
// 	uint8_t event_id; //Event ID may be Alarm-1, Command Execution
 	uint8_t contact_id;// if the sms is a privet reply we must remember sender number
	String number;
}NEW_SMS, *PNEW_SMS ;

//int sms_broadcast_index=0;
//NEW_SMS SMS_to_be_sent_FIXDMEM[10];

typedef struct GPIO_data{
	uint8_t GPIOpin = 0;	
} Struct_GPIO_INFO;

typedef struct Sens_data{
	bool prevState;
	bool nowState;
	bool high_satate_triggerd;	
	long last_changed_time;
} Struct_SENS_INFO;


typedef struct Sensor_name_with_ID{
	char device_name[11];
	char device_rf_id[10];
} SENS_INFO;

typedef struct User_code_and_remote_ID{
	char user_code[5];
	char remote_rf_id[10];
} USER_REMOTE_INFO;

typedef struct my_contact{
	char number[15];
	bool sms_num;
	bool call_num;
	uint8_t security_level;
} GSM_CONTACTS_INFO;

typedef struct Events{
	uint8_t event_type_id;
	uint8_t user_id;
	char discription[15];
	uint8_t 	tm_sec;
	uint8_t 	tm_min;
	uint8_t 	tm_hour;	
	uint16_t tm_year;	
	uint16_t tm_mon;
	uint8_t tm_day;
	
	
} EVENT_INFO;

/* structure that hold data*/
typedef struct{
	char char_buffer_rx[15];
	int counter;
}DataBuffer;

enum GSM_stateMachineStates {GSM_CALL, GSM_INIT,GSM_SMS_SUSPENDING, GSM_LISTIN,GSM_CALL_TASK_DELETEING};

#endif /* TYPEX_H_ */
