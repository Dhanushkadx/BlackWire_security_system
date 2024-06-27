// 
// 
// 

#include "alarm.h"


void ALARM :: get_data(){
	
	for(int i=0;i<40;i++){
		Serial.println(fn_get_sens_name(i));
	}
	
}

void ALARM :: update_rx_rf_id_for_all(const char* rf_id_rx){
	Serial.print(F("rf id from main fn>"));
	Serial.println(rf_id_rx);
	strcpy(recived_rf_id,rf_id_rx);
	rf_id_rx_updated = true;
}

void ALARM :: set_arm_mode(eARM_Mode mode){
	eArm_mode = mode;
}
eARM_Mode ALARM :: get_arm_mode(){
	return eArm_mode;
}

char* ALARM :: get_rf_id_for_sensor(uint8_t sensor_index){
	//return pAny_sensor_array[sensor_index].device_RF_ID;
	return "";// fn_get_sens_rfid(sensor_index);
}

char* ALARM :: get_rf_id_for_remote(uint8_t remote_index){
	//return pAny_sensor_array[sensor_index].device_RF_ID;
	return "";//fn_get_remmote_rfid(remote_index);
}

void ALARM :: set_rf_id_for_remote(uint8_t remote_index, char *remote_id){
	//return pAny_sensor_array[sensor_index].device_RF_ID;
	return fn_set_remote_id(remote_index,remote_id);
}


void ALARM :: set_fn_arm(_callbackFunction pFn){
	fn_arm = pFn;
}


void ALARM :: set_fn_disarm(_callbackFunction pFn){
	fn_disarm = pFn;
}

void ALARM :: set_fn_exit_delay_timer_start(_callbackFunctionType7 pFn){
	fn_exit_delay_timer_start = pFn;
}

void ALARM :: set_fn_intializ_sensors(_callbackFunctionType4 pFn){fn_intializ_sensors = pFn;}
void ALARM :: set_fn_entry_delay_timer_start(_callbackFunctionType7 pFn){fn_entry_delay_timer_start = pFn;}
void ALARM :: set_fn_chime_sound(_callbackFunctionType7 pFn){fn_chime_sound = pFn;};
void ALARM :: set_fn_exit_delay_time_out(_callbackFunctionType3 pFn){fn_exit_delay_time_out = pFn;}
void ALARM :: set_fn_entry_delay_time_out(_callbackFunctionType3 pFn){fn_entry_delay_time_out=pFn;}
void ALARM :: set_fn_creat_alarm_msg(_callbackFunctionType3 pFn){fn_creat_msg = pFn;}

void ALARM :: set_fn_zone_state_update_notify(_callbackFunctionType8 pFn){fn_zone_state_update_notify = pFn;};
void ALARM :: set_fn_alarm_calling(_callbackFunctionType4 pFn){fn_alarm_calling = pFn;}
void ALARM :: set_fn_alarm_snooze(_callbackFunctionType7 pFn){fn_alarm_snooze = pFn; }
void ALARM :: set_fn_alarm_notify(_callbackFunctionType2 pFn){fn_alarm_notify = pFn;}
void ALARM :: set_fn_arm_faild(_callbackFunctionType3 pFn){fn_arm_faild = pFn;}
void ALARM :: set_fn_zone_status_update(_callbackFunctionType3 pFn){fn_chime_zone_notify = pFn;}
void ALARM :: set_fn_get_name(_callbackFunctionType5 pFn){fn_get_sens_name = pFn;}
void ALARM :: set_fn_set_name(_callbackFunctionType6 pFn){fn_set_sens_name = pFn;}
void ALARM :: set_fn_get_rfid(_callbackFunctionType10 pFn){fn_get_sens_rfid = pFn;}
void ALARM :: set_fn_set_rfid(_callbackFunctionType6 pFn){fn_set_sens_rfid = pFn;}
void ALARM :: set_fn_get_remote_id(_callbackFunctionType5 pFn){fn_get_remmote_rfid = pFn;}
void ALARM :: set_fn_set_remote_id(_callbackFunctionType6 pFn){fn_set_remote_id = pFn;}
void ALARM :: set_fn_system_is_not_ready(_callbackFunctionType7 pFn){ fn_system_is_not_ready = pFn;}
void ALARM :: set_fn_system_is_ready(_callbackFunctionType7 pFn){ fn_system_is_ready = pFn;}
void ALARM :: set_fn_arm_can_not_be_done(_callbackFunctionType7 pFn){fn_arm_can_not_be_done = pFn;}
void ALARM :: set_fn_rf_zone_re_enable(_callbackFunctionType2 pFn){fn_set_rf_zone_re_enable = pFn;}
void ALARM :: set_fn_alarm_bell_time_out(_callbackFunctionType4 pFn){fn_alarm_bell_time_out = pFn;}
	
	
void ALARM :: set_sensor_name(uint8_t sensor_index,char* sensor_name){
	fn_set_sens_name(sensor_index,sensor_name);
	
}

void ALARM :: set_sensor_bypassed(uint8_t sensor_index, bool state){
	//pAny_sensor_array[sensor_index].sensor_bypased = state;
	
	if (state)
	{
		pAny_sensor_array[sensor_index].device_state|=(1<<BIT_MASK_BYPASSED);
	} 
	else
	{
		pAny_sensor_array[sensor_index].device_state&=~(1<<BIT_MASK_BYPASSED);
	}
}

void ALARM :: set_sensor_24H(uint8_t sensor_index, bool state){
	if (state)
	{
		pAny_sensor_array[sensor_index].device_type |=(1<<BIT_MASK_24H);	
	} 
	else
	{
		pAny_sensor_array[sensor_index].device_type &=~(1<<BIT_MASK_24H);
	}
	
}

void ALARM :: set_sensor_en_de(uint8_t sensor_index, bool state){
	//pAny_sensor_array[sensor_index].sensor_en = state;
	if (state)
	{
		pAny_sensor_array[sensor_index].device_state |=(1<<BIT_MASK_ENABLE);
	}
	else
	{
		pAny_sensor_array[sensor_index].device_state &=~(1<<BIT_MASK_ENABLE);
	}
}


void ALARM :: set_sensor_exit(uint8_t sensor_index, bool state){
	//clear_exit_zone();
	if (state)
	{
		pAny_sensor_array[sensor_index].device_state |=(1<<BIT_MASK_EXIT_DELAY);
	}
	else
	{
		pAny_sensor_array[sensor_index].device_state &=~(1<<BIT_MASK_EXIT_DELAY);
	}
	
	//pAny_sensor_array[sensor_index].exit_delay_en = true;
	
}

void ALARM :: set_sensor_entry(uint8_t sensor_index, bool state){
	//clear_entry_zone();
	if (state)
	{
		pAny_sensor_array[sensor_index].device_state |=(1<<BIT_MASK_ENTRY_DELAY);
	}
	else
	{
		pAny_sensor_array[sensor_index].device_state &=~(1<<BIT_MASK_ENTRY_DELAY);
	}
}

void ALARM :: clear_exit_zone(){
	for (int i=0; i<TOTAL_DEVICES; i++)
	{
		pAny_sensor_array[i].device_state &=~(1<<BIT_MASK_EXIT_DELAY);
	}
}

char* ALARM :: get_sensor_name(uint8_t index){
	//return pAny_sensor_array[index].device_name;
	return fn_get_sens_name(index);
}

bool ALARM :: is_sensor_enable(uint8_t index){
	if (pAny_sensor_array[index].device_state&(1<<BIT_MASK_ENABLE))
	{
		return 1;
	} 
	else
	{
		return 0;
	}
	//return pAny_sensor_array[index].sensor_en;
	
}

bool ALARM :: is_sensor_exit_zone(uint8_t zone){
	if (pAny_sensor_array[zone].device_state&(1<<BIT_MASK_EXIT_DELAY))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
bool ALARM :: is_sensor_entry_zone(uint8_t zone){
	if (pAny_sensor_array[zone].device_state&(1<<BIT_MASK_ENTRY_DELAY))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

bool ALARM :: is_sensor_24h(uint8_t zone){
	if (pAny_sensor_array[zone].device_type&(1<<BIT_MASK_24H))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

bool ALARM :: is_sensor_RF(uint8_t zone){
	if (pAny_sensor_array[zone].device_type&(1<<BIT_MASK_RF))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

bool ALARM :: is_sensor_bypass(uint8_t index){
	//return pAny_sensor_array[index].sensor_bypased;
	
	if (pAny_sensor_array[index].device_state&(1<<BIT_MASK_BYPASSED))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

bool ALARM :: is_sensor_available(uint8_t index){
	//return pAny_sensor_array[index].sensor_bypased;
	
	if (pAny_sensor_array[index].device_state&(1<<BIT_MASK_AVAILABLE))
	{
		return true;
	}
	else
	{
		return false;
	}
}

int8_t ALARM :: get_exit_zone_availablity(){
	Serial.println("check exit delay zones");
	int8_t ret_val = -1; // no exit zone yet
	for (int index =0; index<TOTAL_DEVICES; index++)
	{
		if (pAny_sensor_array[index].device_state&(1<<BIT_MASK_EXIT_DELAY))//sensor is exit delay
		{
			ret_val = index;
		}
	}
	
	return ret_val;
}
int8_t ALARM :: get_entry_zone_availablity(){
	int8_t ret_val = -1; // no exit zone yet
	for (int index =0; index<TOTAL_DEVICES; index++)
	{
		if (pAny_sensor_array[index].device_state&(1<<BIT_MASK_ENTRY_DELAY))//sensor is exit delay
		{
			return index;
		}
	}
	
	return ret_val;
}

uint8_t ALARM :: get_bell_time_timer_interval(){
	return  Timer_alarm_relay_time_out;
}

uint8_t ALARM :: get_entry_delay_time(){
	return Timer_entry_delay_interval;
}
uint8_t ALARM :: get_exit_delay_time(){
	return Timer_exit_delay_interval;
}

uint8_t ALARM :: set_call_back_sms_loop_status(_callbackFunctionType4 pFn){
	fn_sms_loop_status = pFn;
 return 0;
}

void ALARM :: set_fn_save_event_info(_callbackFunctionType9 pFn){
	fn_save_event_infor = pFn;
}



void ALARM :: clear_entry_zone(){
	for (int i=0; i<TOTAL_DEVICES; i++)
	{
		pAny_sensor_array[i].device_state &=~(1<<BIT_MASK_ENTRY_DELAY);
	}
}


void ALARM :: set_system_state(eMain_state state, eInvoking_source invorker, uint8_t user_id){
	_invorking_device = invorker;
	_eCurrunt_state = state;
	this->user_id=user_id;
}


void ALARM :: set_entry_delay_timer_interval(uint8_t interval){
	Timer_entry_delay_interval = interval;
	Timer_entry_delay.interval = interval*1000;
	
}
void ALARM :: set_exit_delay_timer_interval(uint8_t interval){
	Timer_exit_delay_interval = interval;
	Timer_exit_delay.interval = interval*1000;
	}
	
void ALARM :: set_bell_time_timer_interval(uint8_t interval){
	Timer_alarm_relay_time_out = interval;
	Timer_alarm_relay.interval = interval*1000;
	

}
eMain_state ALARM :: get_system_state(){
	return _eCurrunt_state;
}

void ALARM :: Universal_zone_state_update(uint8_t last_update_sensor_index, bool state){
	
	if (last_update_sensor_index>48)
	{
		Serial.println(F("invalide zone index > 47"));
		return;
	}
	if (last_update_sensor_index<0)
	{
		Serial.println(F("invalide zone index < 0"));
		return;
	}
	this->last_update_sensor_index = last_update_sensor_index;
	sensor_state_updated_to_be_processd = true;
	
	if (state)
	{
		pAny_sensor_array[last_update_sensor_index].device_state|=(1<<BIT_MASK_LAST_STATE);
		
		if (is_sensor_RF(last_update_sensor_index))
		{
			Serial.print(F("RF ZONE!"));
			Timer_RF_zone_reactive_delay.interval = 60000;
			Timer_RF_zone_reactive_delay.previousMillis = millis();
			Timer_RF_zone_reactive_en = true;
		}
	} 
	else
	{
		pAny_sensor_array[last_update_sensor_index].device_state&=~(1<<BIT_MASK_LAST_STATE);
	}
	Serial.print(F("**zone state updated**"));
		Serial.println();
		Serial.printf_P(PSTR("zone id:%d state:%d"),last_update_sensor_index,state);
		Serial.println();
	//last_update_sensor_state  = state;
	//zone_state_update_notify(uint8_t zone0_7, uint8_t zone8_15, uint8_t zone16_23, uint8_t zone24_31, uint8_t zone32_39, uint8_t zone40_47);
	// notufy zone state update
	uint8_t zone0_7 = 0, zone8_15 =0, zone16_23=0, zone24_31=0, zone32_39=0, zone40_47=0;
	
	any_zone_bitmask_parameter_to_bytes(BIT_MASK_LAST_STATE, zone0_7,zone8_15,zone16_23,zone24_31,zone32_39,zone40_47);
	//zone0_7,zone8_15,zone16_23,zone24_31,zone32_39,zone40_47
	
	fn_zone_state_update_notify(zone0_7,zone8_15,zone16_23,zone24_31,zone32_39,zone40_47);
}

void ALARM :: watcher(){
	
	// RF zone reactive timer
	if (Timer_RF_zone_reactive_en)
	{
		if (Timer_RF_zone_reactive_delay.Timer_run())
		{
			Timer_RF_zone_reactive_en = false;
			for (uint8_t index = 0; index<TOTAL_DEVICES; index++)
			{
				if (is_sensor_RF(index))
				{
					pAny_sensor_array[index].device_state&=~(1<<BIT_MASK_LAST_STATE);
					pAny_sensor_array[index].device_state&=~(1<<BIT_MASK_ALARM);
					pAny_sensor_array[index].device_state |=(1<<BIT_MASK_ENABLE);
					fn_set_rf_zone_re_enable(index);
					Serial.println(F("RF zone Reactivated"));
				}
			}
			
		}
	}
	// ALARM RELAY FIRST AUTO TIME OUT
	 if (!_Timer_alarm_relay_time_out)
	 {
		 if (Timer_alarm_relay.Timer_run())
		 {
			 _Timer_alarm_relay_time_out=true;
			 //fn_alarm_bell_time_out();			 
		 }
	 }

	 // ALARM CLEAR DELAY
	 if (Timer_alarm_clear_delay_en)
	 {
		 if (Timer_alarm_clear_delay.Timer_run())
		 {
			Timer_alarm_clear_delay_en = false;
			enable_only_closed_sensors_as_it_is();
			clear_all_sensors_alarm_state();
			Serial.println(F("all zones alarm cleard"));
		 }
	 }
	switch (_eCurrunt_state)
	{
	case DEACTIVE:{
			if (_ePrev_state!=_eCurrunt_state)
			{
				Serial.println(F("SYS_STSTE>DEACTIVE"));
				sensor_state_updated_to_be_processd=true;
				clear_all_sensors_alarm_state();
				//if (_Arm_faild_detected==false)
				if(_ePrev_state!=TRANCITION)
				{
					fn_disarm(user_id, "disarm",_invorking_device);
				}
				_ePrev_state=_eCurrunt_state;
				eArm_mode = USER_SELECT;
				fn_intializ_sensors();
			}
		
			
			if (sensor_state_updated_to_be_processd)
			{
				sensor_state_updated_to_be_processd=false;
				chime_sound();
				//24H sensor eg: fire
				Serial.println(F("24H W sensor eg: fire"));
				alarm_process_wired_24H();
				
				if (is_system_ready_to_arm()!=-1)// system is not ready
				{
					//run system is not ready call beck
					fn_system_is_not_ready();
				}
				else{
					
					// run system is ready call back
					fn_system_is_ready();
				}				
			}
			
			
	}
		break;
	case TRANCITION:{
						if (_ePrev_state!=_eCurrunt_state)
						{
							_ePrev_state =_eCurrunt_state;
							Serial.println(F("SYS_STSTE>TRANCITION"));
							fn_arm_faild("",is_system_ready_to_arm());
							Timer_arm_mode_waiting_time_out.previousMillis=millis();
							Timer_arm_mode_waiting_time_out.interval=15000;
						}
						//time out
						if (Timer_arm_mode_waiting_time_out.Timer_run())
						{
							fn_arm_can_not_be_done();
							_Arm_faild_detected=true;
							_eCurrunt_state=DEACTIVE;
						}
						
						if (eArm_mode == AS_ITIS_BYPASS)
						{
							//bypass all open zones
							// go to sys ideal
							_eCurrunt_state = SYS1_IDEAL;
							Serial.println(F("AS_ITIS_BYPASS"));
							
						} 
						else if (eArm_mode == AS_ITIS_NO_BYPASS)
						{
							//deactivate all open zones
							// go to sys ideal
							_eCurrunt_state = SYS1_IDEAL;
							Serial.println(F("AS_ITIS_NO_BYPASS"));
						}
						
	}
	break;
	case SYS1_IDEAL:{
		if (_ePrev_state!=_eCurrunt_state)
		{
			Serial.println(F("SYS_STSTE>SYS_IDEAL"));	
			
			if (_ePrev_state==BEGING)// may be after power cut
			{
				eArm_mode=AS_ITIS_NO_BYPASS;
			}
			if (_ePrev_state==ALARM_CALLING)
			{
				Serial.println(F("alarm snoozed"));
				clear_all_sensors_alarm_state();	
				fn_alarm_snooze();
				_exit_delay_timer_en = false;
			}
			
			else if ((_ePrev_state==DEACTIVE)||(_ePrev_state==BEGING)||(_ePrev_state==TRANCITION))
			{
						
					clear_all_sensors_alarm_state();	
					//update sensors again
					// request all sensor update;
					
					fn_intializ_sensors();
					//				
					//check for open sensors before arm
					if ((is_system_ready_to_arm()!=-1)&&(eArm_mode==USER_SELECT))
					{
						_ePrev_state =_eCurrunt_state;
						//fn_arm_faild("",is_system_ready_to_arm());
						
						_eCurrunt_state=TRANCITION;
						return;
						
					}
					else{// all sensors are closed at this line
						//look for exit delay
						_Arm_faild_detected=false;
						Serial.println("_Arm_faild_detected=false");
						int8_t exit_zone = get_exit_zone_availablity();
						if (exit_zone!=-1)
						{
							Serial.println(exit_zone);
							Serial.println(F("enable exit timer"));
							_exit_delay_timer_en=true;
							Timer_exit_delay.previousMillis = millis();
							Timer_exit_delay.interval = Timer_exit_delay_interval*1000;
							fn_exit_delay_timer_start();
						}
						
						enable_only_closed_sensors_as_it_is();
						fn_arm(user_id, "sensor",_invorking_device);
					}	
			  }
						
			
			
			_ePrev_state =_eCurrunt_state;
		
		
		}
		if (sensor_state_updated_to_be_processd)
		{
			sensor_state_updated_to_be_processd=false;
			
			alarm_process_wired();					
			
		}
		
		
		
		//exit time
		if (_exit_delay_timer_en)
		{
			//Serial.println("exit delay en");
			
			if (Timer_exit_delay.Timer_run())
			{
				_exit_delay_timer_en = false;
				
				for (int index=0; index<TOTAL_DEVICES; index++)
				{
					if (pAny_sensor_array[index].device_state&(1<<BIT_MASK_EXIT_DELAY))
					{
						if (pAny_sensor_array[index].device_state&(1<<BIT_MASK_LAST_STATE))
						{
							//exit delay time out and still not close
							Serial.print(F("sensor exit delay>"));
							Serial.println(index);
							fn_alarm_notify(index);
							fn_exit_delay_time_out("Exit",index);
							return;
							
						}
					}
				}
				//tun off buzzer and stay on arm state				
				fn_exit_delay_time_out("Exit",-1);
				//eCurrunt_state = ALARM_CALLING;
				//sendOnI2C(8,msg,reply,strlen(msg));
			}
		}
		
		if (_entry_delay_timer_en)
		{
			Timer_RF_zone_reactive_en = false;
			
			if (Timer_entry_delay.Timer_run())
			{
				Serial.print(F("Entry delay time out in "));//Timer_entry_delay.interval
				Serial.println(Timer_entry_delay.interval);
				_entry_delay_timer_en = false;
				// check last time pending alarms
				for (int index =0; index<TOTAL_DEVICES; index++)
				{
					if (pAny_sensor_array[index].device_state&(1<<BIT_MASK_ALARM))
					{						
						Timer_RF_zone_reactive_en = true;
						fn_alarm_notify(index);
						_eCurrunt_state =ALARM_CALLING;
						Serial.print(F("alarm detected>> "));
						fn_entry_delay_time_out("no msg",_invorking_device);
					}
				}
			}
			
		}
	}
	break;
	
	case ALARM_SMS_SENDING:{
		if (_ePrev_state!=_eCurrunt_state)
		{
			_ePrev_state=_eCurrunt_state;
			Serial.println(F("SYS_STSTE>SMS_SENDING"));
			
			
		}
		//if (fn_sms_loop_status()==0)
		//{
			_eCurrunt_state = ALARM_CALLING;
		//}
		//fn_alarm_ocalling();
		
	}
	break;
	
	case ALARM_CALLING:{
		if (_ePrev_state!=_eCurrunt_state)
		{
			_ePrev_state=_eCurrunt_state;
			Serial.println(F("SYS_STSTE>ALARM_CALLING"));
			Timer_alarm_relay.previousMillis=millis();
			_Timer_alarm_relay_time_out=false;
			uint8_t result = fn_alarm_calling();
		}
		
		 if (sensor_state_updated_to_be_processd)
		 {
			 sensor_state_updated_to_be_processd=false;
			
			 alarm_process_wired();
			
		 }
		
	}
	break;	
	}
	
	};
	
bool ALARM :: set_rf_id_for_sensor(char* id, uint8_t index){
	bool ret_val = true;
	for (int i=RF_DEVICE_START_INDEX; i<TOTAL_DEVICES; i++)
	{
		//if (strncmp(id,fn_get_sens_rfid(i),7)==0)
		//{
			//return ret_val;
		//}
	}
	ret_val = false;
	fn_set_sens_rfid(index,id);	
	return ret_val;
}

	
	
int8_t ALARM :: get_absolute_zone_number(eInvoking_source card_id, uint8_t relative_zone_number){
		
		switch (card_id)
		{
			case CARD_A:{
				return relative_zone_number;
			}
			break;
			case CARD_B:{
				return relative_zone_number+card_A_zone__starting_number;
			}
			break;
			case CARD_C:{
				return relative_zone_number+card_A_zone__starting_number + card_B_zone__starting_number;
			}
			break;
			default:{Serial.println(F("unknown card id")); return -1;}
		}
	}
	
// No arg to pass all variables are class members
	
void ALARM :: alarm_process_wired(){


	for(uint8_t scanning_index=0; scanning_index<TOTAL_DEVICES; scanning_index++){
		//#ifdef _DEBUG		
			Serial.printf_P(PSTR("Pro ID>>%d "),scanning_index);				
//#endif
		// zone is bypassed currntly
		if((pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_BYPASSED))){
			Serial.println(F("BYPASSED"));
			 continue; // Skip the rest of the code and continue with the next iteration
			 }

		if((pAny_sensor_array[scanning_index].device_type&(1<<BIT_MASK_RF))){
			Serial.print(F("RF_ZONE."));
		}
		// zone is not available 
		else if(!(pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_AVAILABLE))){ 
			Serial.println(F("UNAVAILABLE."));
			continue; // Skip the rest of the code and continue with the next iteration
			}
		
		
		bool alarm_enable = false;


			if (pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_LAST_STATE))// the sensor is opened
			{	
				Serial.print(F(">>OPEND"));

			if(pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_ENABLE))
			{
				
				Serial.print(F(">>>Enable/"));
				
				//the sensor is NOT an entry delay or exit delay 
				if((!(pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_ENTRY_DELAY)))&&(!(pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_EXIT_DELAY))))
				{
					Serial.print(F("ENTRY-NO/EXIT-NO"));
					//No previous alarm
					if((!(pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_ALARM)))&&((pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_ENABLE))))
					{
						pAny_sensor_array[scanning_index].device_state|=(1<<BIT_MASK_ALARM);
						Serial.print(F(">ALARM ENABLED"));
						alarm_enable = true;						
					}
					else{
						Serial.println(F(">ALREADY ALARM OR DISABLE"));
					}
				}	
				//the sensor is both  entry and exit delay 
				else if((pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_ENTRY_DELAY))&&(pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_EXIT_DELAY))){
					Serial.print(F("ENTRY-YES/EXIT-YES"));
					
					if (!_exit_delay_timer_en)// if exit delay timer is NOT running
					{
						//No previous alarm //the sensor is an entry delay so activate the timer
						if (!_entry_delay_timer_en)
						{
							Serial.print(F("ENTRY-YES"));
							// do not go to alarm sate just enable the timer
							pAny_sensor_array[scanning_index].device_state|=(1<<BIT_MASK_ALARM);
							_entry_delay_timer_en=true;
							fn_entry_delay_timer_start();
							Timer_entry_delay.previousMillis = millis();
						}
						else{
							Serial.println(F(">ALREADY ALARM OR DISABLE"));
						}
					}
					else{// if exit delay timer is running
							Serial.print(F("EXIT-TIMER RINNING"));
					}
					
				}
				//the sensor is ONLY entry delay 
				else if ((pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_ENTRY_DELAY))&&(!(pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_EXIT_DELAY))))
				{
					Serial.print(F("ENTRY-YES/EXIT-NO"));
					//No previous alarm //the sensor is an entry delay so activate the timer
					if((!(pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_ALARM)))&&((pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_ENABLE))))
					{
						if (!_entry_delay_timer_en)
						{
							Serial.println(F("ENTRY TIMER ENABLED"));
							pAny_sensor_array[scanning_index].device_state|=(1<<BIT_MASK_ALARM);
							_entry_delay_timer_en=true;
							Timer_entry_delay.previousMillis = millis();
						}
						
						if (_exit_delay_timer_en)
						{
							Serial.println(F("entry delay only zone trigger when exit delay timer running"));
							pAny_sensor_array[scanning_index].device_state|=(1<<BIT_MASK_ALARM);
							alarm_enable=true;
						}
					}
					else{
						Serial.println(F("ALREADY ALARM OR DISABLED"));
					}
				}
				// the sensor is ONLY exit delay
				else if ((!(pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_ENTRY_DELAY)))&&((pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_EXIT_DELAY))))
				{
					Serial.print(F("ENTRY-NO/EXIT-YES"));
					//No previous alarm //the sensor is an entry delay so activate the timer
					if((!(pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_ALARM)))&&((pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_ENABLE))))
					{
						if (!_exit_delay_timer_en)
						{
							Serial.println(F("exit delay only zone trigger when exit delay timer running is NOT running"));
							pAny_sensor_array[scanning_index].device_state|=(1<<BIT_MASK_ALARM);
							alarm_enable=true;
						}
					}
					else{
						
					}
				}
			}//enable check
			else{Serial.println(F(">>>DISABLED/"));}
			}//sensor is open
			
			else{// sensor is just closed
				// check arm mode
				Serial.print(F(">>CLOSED"));
				Serial.println(F(">>>enable is if the syetem is AS IT IS"));
				if (eArm_mode==AS_ITIS_NO_BYPASS)
				{
					pAny_sensor_array[scanning_index].device_state|=(1<<BIT_MASK_ENABLE);
					//Serial.println(F("sensor is enabled"));
					if (is_system_ready_to_arm()!=-1)// system is not ready
					{
						//run system is not ready call beck
						fn_system_is_not_ready();
					}
					else{						
						// run system is ready call back
						fn_system_is_ready();
					}
				}				
				
			}	
		
			/*if (millis()-pAny_sensor_array[last_update_sensor_index].last_updated_time_stamp<500)
			{
				Serial.println(F("too short"));
				return;
			}*/
			
			if (alarm_enable)
			{
				alarm_enable=false;
			
			pAny_sensor_array[scanning_index].last_updated_time_stamp=millis();			
			// check perimeter only
			if (perimeter_only)
			{
				if (pAny_sensor_array[scanning_index].device_type&(1<<BIT_MASK_PERIMETER))
				{
					_eCurrunt_state =ALARM_CALLING;
					Timer_alarm_relay.previousMillis=millis();
					_Timer_alarm_relay_time_out=false;
					pAny_sensor_array[scanning_index].device_state|=(1<<BIT_MASK_ALARM);
					fn_alarm_notify(scanning_index);
					Timer_alarm_clear_delay.interval = 60000;
					Timer_alarm_clear_delay.previousMillis = millis();
					Timer_alarm_clear_delay_en = true;
					Serial.print(F("alarm detected perimeter only>> "));
				}
				else{
					Serial.print(F("not a perimeter zone alarm canceled "));
				}
				
			}
			else{
				_eCurrunt_state =ALARM_CALLING;
				pAny_sensor_array[scanning_index].device_state|=(1<<BIT_MASK_ALARM);
			    fn_alarm_notify(scanning_index);
				Timer_alarm_relay.previousMillis=millis();
				_Timer_alarm_relay_time_out=false;
				Serial.print(F("alarm detected all arm>> "));
				Timer_alarm_clear_delay.interval = 60000;
				Timer_alarm_clear_delay.previousMillis = millis();
				Timer_alarm_clear_delay_en = true;
			}
			}
		
	}
	
}
		
	
void ALARM :: enable_only_closed_sensors_as_it_is(){

	for (int scanning_index=0; scanning_index<TOTAL_DEVICES; scanning_index++){
	
//#ifdef _DEBUG		
		Serial.printf_P(PSTR("Zone ID>>%d "),scanning_index);				
//#endif
		// zone is bypassed currntly
		if((pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_BYPASSED))){
			Serial.println(F("BYPASSED"));
			 continue; // Skip the rest of the code and continue with the next iteration
			 }

		if((pAny_sensor_array[scanning_index].device_type&(1<<BIT_MASK_RF))){
			Serial.print(F("RF_ZONE."));
		}
		// zone is not available 
		else if(!(pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_AVAILABLE))){ 
			Serial.println(F("UNAVAILABLE."));
			continue; // Skip the rest of the code and continue with the next iteration
			}	
				
				
			if (pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_LAST_STATE))// if the sensor is opened
			{
				pAny_sensor_array[scanning_index].device_state&=~(1<<BIT_MASK_ENABLE);// disable the enable flag
				Serial.println(F("DISABLE"));
			
			}
			else{
				pAny_sensor_array[scanning_index].device_state|=(1<<BIT_MASK_ENABLE);
				Serial.println(F("ENABLE"));
			}
					
			/*if (pAny_sensor_array[i].device_type&(1<<BIT_MASK_RF))
			{
				pAny_sensor_array[i].device_state|=(1<<BIT_MASK_ENABLE);
				Serial.println(F("RF sensor Enable"));
			}*/
				
			}
}
int8_t ALARM :: is_system_ready_to_arm(){
	
	int8_t ret_val = -1;
	for (int i=0; i<TOTAL_DEVICES; i++)
			{
				
				if ((pAny_sensor_array[i].device_state&(1<<BIT_MASK_LAST_STATE))&&(!(pAny_sensor_array[i].device_state&(1<<BIT_MASK_BYPASSED))))
				{
					//pAny_sensor_array[i].sensor_en = false;
					ret_val = i;
					return ret_val;
					
				}
				
			}
		return ret_val;
}
void ALARM :: clear_all_sensors_alarm_state(){
	Serial.println(F("All sensors alarm clear"));
	for (int i=0; i<TOTAL_DEVICES; i++)
			{
				pAny_sensor_array[i].device_state&=~(1<<BIT_MASK_ALARM);
				
				
			}
}

void ALARM :: alarm_process_wired_24H(){
		
	if ((pAny_sensor_array[last_update_sensor_index].device_state&(1<<BIT_MASK_BYPASSED)))
	{
		return;
	}
	if ((pAny_sensor_array[last_update_sensor_index].device_state&(1<<BIT_MASK_LAST_STATE))&&(pAny_sensor_array[last_update_sensor_index].device_type&(1<<BIT_MASK_24H)))// opend
	{
		pAny_sensor_array[last_update_sensor_index].last_updated_time_stamp=millis();
		
		pAny_sensor_array[last_update_sensor_index].device_state|=(1<<BIT_MASK_ALARM);
		fn_alarm_notify(last_update_sensor_index);
		_eCurrunt_state =ALARM_CALLING;
	
	}
	else if (pAny_sensor_array[last_update_sensor_index].device_state&(1<<BIT_MASK_LAST_STATE))
	{
		fn_chime_zone_notify("",last_update_sensor_index);
	}
	
}

bool ALARM :: is_sensor_ready(uint8_t zone){
	
	if(!(pAny_sensor_array[zone].device_type&(1<<BIT_MASK_RF))){
		
		if ((pAny_sensor_array[zone].device_state&(1<<BIT_MASK_LAST_STATE))&&(!(pAny_sensor_array[zone].device_state&(1<<BIT_MASK_BYPASSED))))
		{
			return false;
		}
		else{
			return true;
		}
	}
	else{
		return true;
	}
	

	
	
}

//check any attribute for all sensors and format them to byte array to transmite on can or any
//arguments: 6 byte addresses to stor sensor data of 48 total ( 8x 6 = 48), and bitmask number in uint8 value.
void ALARM :: any_zone_bitmask_parameter_to_bytes(uint8_t bit_mask, uint8_t& zone0_7, uint8_t& zone8_15, uint8_t& zone16_23, uint8_t& zone24_31, uint8_t& zone32_39, uint8_t& zone40_47){
	
	uint8_t bit_mask_a = 0, bit_mask_b = 0, bit_mask_c = 0, bit_mask_d = 0, bit_mask_e = 0, bit_mask_f = 0;
	for (uint8_t index=0; index<48; index++)
	{
		
			if ((0<=index)&&(index<8))
			{
				//Serial.println(F("process 8 to 15"));
				if (pAny_sensor_array[index].device_state&(1<<bit_mask))
				{
					zone0_7|=(1<<bit_mask_a);
				}
				else{
					zone0_7&=~(1<<bit_mask_a);
				}
				
				bit_mask_a++;
			} 
			else if ((8<=index)&&(index<16))
			{
				//Serial.println(F("process 8 to 15"));
				if (pAny_sensor_array[index].device_state&(1<<bit_mask))
				{
					zone8_15|=(1<<bit_mask_b);
				}
				else{
					zone8_15&=~(1<<bit_mask_b);
				}
				bit_mask_b++;
			}
			else if ((16<=index)&&(index<24))
			{
				//Serial.println(F("process 16 to 23"));
				if (pAny_sensor_array[index].device_state&(1<<bit_mask))
				{
					zone16_23|=(1<<bit_mask_c);
				}
				else{
					zone16_23&=~(1<<bit_mask_c);
				}
				bit_mask_c++;
			}
			else if ((24<=index)&&(index<32))
			{
				//Serial.println(F("process 24 to 31"));
				if (pAny_sensor_array[index].device_state&(1<<bit_mask))
				{
					zone24_31|=(1<<bit_mask_d);
				}
				else{
					zone24_31&=~(1<<bit_mask_d);
				}
				bit_mask_d++;
			}
			else if ((32<=index)&&(index<40))
			{
				//Serial.println(F("process 32 to 39"));
				if (pAny_sensor_array[index].device_state&(1<<bit_mask))
				{
					zone32_39|=(1<<bit_mask_e);
				}
				else{
					zone32_39&=~(1<<bit_mask_e);
				}
				bit_mask_e++;
			}
			else if ((40<=index)&&(index<48))
			{
				//Serial.println(F("process 40 to 47"));
				if (pAny_sensor_array[index].device_state&(1<<bit_mask))
				{
					zone40_47|=(1<<bit_mask_f);
				}
				else{
					zone40_47&=~(1<<bit_mask_f);
				}
				bit_mask_f++;
			}
		
	}
}


void ALARM :: chime_sound(){
	uint8_t scanning_index = last_update_sensor_index;
		// zone is bypassed currntly
		if((pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_BYPASSED))){			
			 return; // Skip the rest of the code and continue with the next iteration
			 }
		// zone is not available 
		if(!(pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_AVAILABLE))){ 			
				if((!pAny_sensor_array[scanning_index].device_type&(1<<BIT_MASK_RF))){					
					return; // Skip the rest of the code and continue with the next iteration
				}			
			}
		// zone is silent
		else if(!(pAny_sensor_array[scanning_index].device_type&(1<<BIT_MASK_SILENT))){
			Serial.println(F("NO CHIME"));
			return; // Skip the rest of the code and continue with the next iteration
			 }
		if (pAny_sensor_array[scanning_index].device_state&(1<<BIT_MASK_LAST_STATE))// if the sensor is opened
			{
				fn_chime_sound(); 			
			}
			else{				
			}
}