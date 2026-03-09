// alarm.h

#ifndef _ALARM_h
#define _ALARM_h
#include "typex.h"
#include "TimerSW.h"
#include "Arduino.h"
#include "logger.h"

extern "C" {
	typedef void (*_callbackFunction)(uint8_t,const char*,eInvoking_source);
	typedef void (*_callbackFunctionType2)(uint8_t);
	typedef bool (*_callbackFunctionType3)(const char*,int);
	typedef uint8_t (*_callbackFunctionType4)(void);
	typedef char* (*_callbackFunctionType5)(uint8_t);
	typedef void (*_callbackFunctionType6)(uint8_t,const char*);
	typedef void (*_callbackFunctionType7)(void);
	typedef void (*_callbackFunctionType8)(uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t);
	/*uint8_t event_type_id;
	uint8_t user_id;
	char time_infor[25];
	char discription[15];*/
	typedef void (*_callbackFunctionType9)(uint8_t,uint8_t,char*,char*);
	typedef int8_t(*_callbackFunctionType10)(const char*);
	
};


typedef enum arm_mode{
	USER_SELECT=0,
	AS_ITIS_NO_BYPASS,
	AS_ITIS_BYPASS,
	
	} eARM_Mode;

#define card_A_zone__starting_number 8
#define card_B_zone__starting_number 16
class ALARM{
	public:
			ALARM(MY_SENS* mystruct){
				pAny_sensor_array = mystruct;				
				fn_arm = NULL;
				rf_id_rx_updated = false;
				_ePrev_state = BEGING;
				};
				
			// continuous running loop for alarm class
			void watcher();
			// Read system state
			eMain_state	get_system_state();
			// print all zone names from EEPROM
			void get_data();
			// Set system state
			void set_system_state(eMain_state state, eInvoking_source invorker, uint8_t user_id);
			
			void any_zone_bitmask_parameter_to_bytes(uint8_t bit_mask_of_para, uint8_t& zone0_7, uint8_t& zone8_15, uint8_t& zone16_23, uint8_t& zone24_31, uint8_t& zone32_39, uint8_t& zone40_47);
			// set any zone state
			void Universal_zone_state_update(uint8_t last_update_sensor_index, bool state);
			
			
			//if the state changed zone is wired this function will check for alarms and callers call back functions
			void alarm_process_wired();		
			void alarm_process_wired_24H();			
			//Main board has only 8 physical zone ports. others are on expansion cards. each card has an address. Zone data structure use absolute zone number
			//each card sends relative zone number we should map relative zone number to absolute zone number based on the card address.
			int8_t get_absolute_zone_number(eInvoking_source card_id, uint8_t relative_zone_number);
			
		
			void update_rx_rf_id_for_all(const char* rf_id_rx);			
			
			// RF zone id received and rf is binds to given zone only for wireless sensors see RF_DEVICE_START_INDEX offset for wireless zones
			bool set_rf_id_for_sensor(char* id, uint8_t index);
			char* get_rf_id_for_sensor(uint8_t sensor_index);
			
			char* get_rf_id_for_remote(uint8_t remote_index);
			void set_rf_id_for_remote(uint8_t remote_index, char *remote_id);			
			////////////HAPPENS WHEN DISARMING/////////////////////////////////////
			///////////////////////////////////////////////////////////////////////
			//calls when goes to Disarm
			void set_fn_disarm(_callbackFunction pFn);
			//only if there is exit delay zones this fn will be called
			void set_fn_exit_delay_timer_start(_callbackFunctionType7 pFn);
			//////////////////////////////////////////////////////////////////////////
			
			/////////////HAPPENS WHEN ARMING//////////////////////////////////////
			///////////////////////////////////////////////////////////////////////
			// at the beginning scan all sensor state and update the status of each sensors
			void set_fn_intializ_sensors(_callbackFunctionType4 pFn);			
			//only if there is entry delay zones this fn will be called
			void set_fn_entry_delay_timer_start(_callbackFunctionType7 pFn);
			//chime sound hardware function
			void set_fn_chime_sound(_callbackFunctionType7 pFn);
			//when arming, user will be asked to bypass or not any open zones if used did not answer in the given time this fn will be called.
			void set_fn_arm_faild(_callbackFunctionType3 pFn);
			// Will be called when exit delay time out. responsible to check whether the exit delay zone is still open or not and handle the situation.
			void set_fn_exit_delay_time_out(_callbackFunctionType3 pFn);
			//system_userRequest_timeOut_broadcast_OnCAN
			void set_fn_arm_can_not_be_done(_callbackFunctionType7 pFn);
			//calls if ARMING is success
			void set_fn_arm(_callbackFunction pFn);
			///////////////////////////////////////////////////////////////////////////
			
			
			////////////HAPPENS WHEN SENSOR STATE CHENGE/////////////////////////////
			/////////////////////////////////////////////////////////////////////////
			// see inside of alarm_process_wired_24H();
			// Only used to inform chime alarm using i2c to MCU2. zone state update on CAN bus will be sent separately. see >call_back_zone_state_update_notify();
			void set_fn_zone_status_update(_callbackFunctionType3 pFn);
			//update zone state change on CAN bus , rs485 or rs232 will be sent separately			
			void set_fn_zone_state_update_notify(_callbackFunctionType8 pFn);
			//will be triggered is the all zones are closed after any zone change
			void set_fn_system_is_not_ready(_callbackFunctionType7 pFn);
			//will be triggered there is any opend zone after any zone change, bypassed zones will not be considered.
			void set_fn_system_is_ready(_callbackFunctionType7 pFn);
			//if a zone opend in ARM state , and if it is on entry delay 
			void set_fn_entry_delay_time_out(_callbackFunctionType3 pFn);
			// re enable rf zone 
			void set_fn_rf_zone_re_enable(_callbackFunctionType2 pFn);
			///////////////////////////////////////////////////////////////////////////
			
			////////////HAPPENS WHEN ALRM TRIGGERD/////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////
			//responsible for calling all GSM numbers via a separate demon function
			void set_fn_alarm_calling(_callbackFunctionType4 pFn);
			// NOT USED : responsible for creating SMS msg to be send via separate SMS demon function
			void set_fn_creat_alarm_msg(_callbackFunctionType3 pFn);
			//creating SMS msg to be send via separate SMS demon function and ***CAN bus also
			void set_fn_alarm_notify(_callbackFunctionType2 pFn);
			//if a user acknowledge the alarm and want to stay ARM state exiting alarm calling should be stooped and calling demon function should be suspend.
			void set_fn_alarm_snooze(_callbackFunctionType7 pFn);
			
			uint8_t set_call_back_sms_loop_status(_callbackFunctionType4 pFn);
			///////////////////////////////////////////////////////////////////////////////
			
			///////////////INFORMATION READ OR WRITE/////////////////////////////////////
			/////////////////////////////////////////////////////////////////////////////
			// responsible off external siren after time out 
			void set_fn_alarm_bell_time_out(_callbackFunctionType4 pFn);
			// zone information read/write functions
			void set_fn_get_name(_callbackFunctionType5 pFn);
			void set_fn_set_name(_callbackFunctionType6 pFn);
			void set_fn_get_rfid(_callbackFunctionType10 pFn);
			void set_fn_get_remote_id(_callbackFunctionType5 pFn);
			void set_fn_set_remote_id(_callbackFunctionType6 pFn);
			void set_fn_set_rfid(_callbackFunctionType6 pFn);			
			void set_sensor_name(uint8_t sensor_index,char* sensor_name);
			void set_sensor_bypassed(uint8_t sensor_index, bool state);
			void set_sensor_24H(uint8_t sensor_index, bool state);
			void set_sensor_en_de(uint8_t sensor_index, bool state);
			void set_sensor_entry(uint8_t sensor_index, bool state);
			void set_sensor_exit(uint8_t sensor_index, bool state);
			void clear_entry_zone();
			void clear_exit_zone();
			bool is_sensor_enable(uint8_t index);
			bool is_sensor_bypass(uint8_t index);
			bool is_sensor_available(uint8_t index);
			bool is_sensor_ready(uint8_t zone);
			bool is_sensor_24h(uint8_t zone);
			bool is_sensor_RF(uint8_t zone);
			bool is_sensor_exit_zone(uint8_t zone);
			bool is_sensor_entry_zone(uint8_t zone);
			char* get_sensor_name(uint8_t index);
			int8_t get_exit_zone_availablity();
			int8_t get_entry_zone_availablity();
			uint8_t get_entry_delay_time();
			uint8_t get_exit_delay_time();
			uint8_t get_bell_time_timer_interval();
			
			void set_entry_delay_timer_interval(uint8_t interval);
			void set_exit_delay_timer_interval(uint8_t interval);
			void set_bell_time_timer_interval(uint8_t interval);
			void set_fn_system_is_not_ready();
			void set_fn_system_is_ready();
			
			int8_t is_system_ready_to_arm();
			void set_arm_mode(eARM_Mode mode);
			eARM_Mode get_arm_mode();
			void set_fn_save_event_info(_callbackFunctionType9 pFn);
			void chime_sound();

	// ==================== FUNCTION PROTOTYPES ====================
bool is_sensor_skipped(uint8_t index);
void process_open_sensor(uint8_t index, bool &alarm_enable);
void process_closed_sensor(uint8_t index);
void handle_alarm_trigger(uint8_t index);

	private:
	eARM_Mode eArm_mode;
	MY_SENS *pAny_sensor_array;
	//uint8_t latest_update
	bool sensor_state_updated_to_be_processd;
	eMain_state _eCurrunt_state;
	eMain_state _ePrev_state = BEGING;
	_callbackFunction fn_arm,fn_disarm;

	_callbackFunctionType2 fn_zone_time_out;
	_callbackFunctionType3 fn_arm_faild;
	_callbackFunctionType3 fn_chime_zone_notify;
	
	
	_callbackFunctionType3	fn_creat_msg,fn_entry_delay_time_out, fn_exit_delay_time_out;
	_callbackFunctionType4  fn_intializ_sensors;
	_callbackFunctionType4	fn_alarm_calling;
	_callbackFunctionType4  fn_sms_loop_status;
	_callbackFunctionType4  fn_alarm_bell_time_out;
	_callbackFunctionType5  fn_get_sens_name, fn_get_remmote_rfid;
	_callbackFunctionType6  fn_set_sens_name, fn_set_sens_rfid, fn_set_remote_id;
	_callbackFunctionType2  fn_alarm_notify;
	_callbackFunctionType7  fn_system_is_not_ready, fn_system_is_ready, fn_arm_can_not_be_done,fn_alarm_snooze, fn_exit_delay_timer_start, fn_entry_delay_timer_start,fn_chime_sound;
	_callbackFunctionType8  fn_zone_state_update_notify;
	_callbackFunctionType9  fn_save_event_infor;
	_callbackFunctionType10 fn_get_sens_rfid;
	_callbackFunctionType2 fn_set_rf_zone_re_enable;
	bool rf_id_rx_updated;
	char recived_rf_id[10];
	bool last_update_sensor_state;
	int8_t last_update_sensor_index;
	eInvoking_source _invorking_device;
	uint8_t user_id;
	
	bool _exit_delay_timer_en;
	bool _exit_delay_time_out;
	bool _entry_delay_time_out;
	bool _entry_delay_timer_en;
	bool _Timer_alarm_relay_time_out=true;
	bool _Arm_faild_detected;
	bool perimeter_only;
	bool Timer_RF_zone_reactive_en = false;
	bool Timer_alarm_clear_delay_en = false;
	
	TimerSW Timer_exit_delay;
	TimerSW Timer_RF_zone_reactive_delay;
	TimerSW Timer_alarm_clear_delay;
	TimerSW Timer_arm_mode_waiting_time_out;
	TimerSW Timer_entry_delay;
	TimerSW Timer_alarm_relay;
	uint8_t Timer_exit_delay_interval;
	uint8_t Timer_entry_delay_interval;
	uint8_t Timer_alarm_relay_time_out;
	
	void enable_only_closed_sensors_as_it_is();
	void clear_all_sensors_alarm_state();
	};

#endif
