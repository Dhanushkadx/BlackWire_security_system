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

// Compile-time force mode values — use with build flag: -D FORCE_SYS_MODE=N
// Example in platformio.ini: build_flags = ... -D FORCE_SYS_MODE=FORCE_MODE_CONFIG
#define FORCE_MODE_CONFIG    0   // CONFIG_MODE
#define FORCE_MODE_WIFI      1   // NOMAL_MODE_WIFI
#define FORCE_MODE_NO_WIFI   2   // NOMAL_MODE_NO_WIFI

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

enum eSiren_state{SIREN_ON,SIREN_OFF,SIREN_OFF_ITSELF,SIREN_MOMENT,SIREN_CONTINUOUS,SIREN_WAIT_INTERVAL};

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
	uint16_t beep_time_out;
	bool beep_en;
	bool siren_en;
	eMain_state last_system_state;
	char wifissid_ap[25];  //":"dxdxdxdxdx",
	char wifissid_sta[25]; //":"dxdxdxdxdx",
	char wifipass[25];     //": "xxxxxxxxxx",  (STA password)
	char wifipass_ap[25];  // AP password (wapPw in config)
	bool wifi_sta_en;
	bool wifiap_en;
	bool mqtt_en;
	uint8_t call_attempts;//": 2,
	boolean call_en;  //": true,
	char inst_no[25];
	char inst_pas[25];
	bool gsm_module_enable;
	// Entry / exit delay feature flags
	bool et_en;    // entry delay enabled
	bool et_beep;  // beep during entry delay
	bool xt_en;    // exit delay enabled
	bool xt_beep;  // beep during exit delay
	// Boot arm state — "disarm" / "arm" / "away"
	char sys_mode[10];
#ifdef CUSTOM_NETWORK_CONFIG
	char wbssid[18]; // target AP MAC e.g. "84:AF:EC:11:22:33"
#endif
	// Local (non-secure) MQTT — ignored when MQTT_SECURE is defined
	char mqtt_server[64];
	uint16_t mqtt_port;
	char mqtt_user[32];
	char mqtt_pass[32];
	uint32_t config_ver;
	uint64_t config_updated_ts;

} systemConfigTypedef_struct;

#define DEVICE_NAME_MAX_LENGTH 10// my door
#define TOTAL_DEVICES 48
constexpr uint8_t ZONE_COUNT = TOTAL_DEVICES;
#define RF_DEVICE_START_INDEX 0
typedef struct sensorAtribute{
	uint8_t device_state;	
	uint8_t device_type;
	uint8_t device_card_id;
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
	char msg_content[160];
	uint8_t type; //group sms-1, privet reply-2, notification phones only-3
// 	uint8_t event_id; //Event ID may be Alarm-1, Command Execution
 	uint8_t contact_id;// if the sms is a privet reply we must remember sender number
	char number[20];
}NEW_SMS, *PNEW_SMS;

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
}DataBuffer;


typedef struct SensorStatePacket {
    uint8_t zoneStates[6];  // 48 sensors = 6 bytes
}ByteBuffer;

enum GSM_stateMachineStates {GSM_CALL, GSM_INIT,GSM_SMS_SUSPENDING, GSM_LISTIN,GSM_CALL_TASK_DELETEING};

#endif /* TYPEX_H_ */
