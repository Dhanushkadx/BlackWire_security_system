

#include "call_backs.h"


NEW_SMS SMS_to_be_sent_FIXDMEM[SMS_STRCUT_MAX_MGS];
USER_REMOTE_INFO STRUCT_user_remote_infor;
GSM_CONTACTS_INFO STRUCT_GSM_contact_infor;
SENS_INFO STRUCT_sens_infor;

uint8_t call_back_alarm_Calling(){
		uint8_t ret_val = 1;
		publish_system_state("TRIGGERD","info/mode",true);	
		xEventGroupSetBits(EventRTOS_buzzer,    TASK_2_BIT );
#ifdef GSM_OK	
		// check if calling enable flag
		if(systemConfig.call_en==true){
			//check if there is alradey calling
			EventBits_t uxBits = xEventGroupGetBits( EventRTOS_gsm );
			
			if(  ( uxBits & TASK_6_BIT ) != 0  )// alradey calling
			{
			/* if counter is equal 2 then set event bit of task1 */
			}
			else{
				/* Set bit 0 and bit 4 in xEventGroup. */
				xEventGroupSetBits(EventRTOS_gsm,    TASK_1_BIT );// call task invoking request
				/* Set bit 0 and bit 4 in xEventGroup. */
				xEventGroupClearBits(EventRTOS_gsm,    TASK_2_BIT );
				ret_val = 0;
			}
		}		
#endif		
		//eCrrunt_lcd_page = ALARMING;		
		// set event for lcd
		EventBits_t uxBits_lcd;		
		uxBits_lcd = xEventGroupSetBits(EventRTOS_lcd,    TASK_3_BIT );		
		return ret_val;
}

bool call_back_creat_alarm_sms(const char* buffer_sms, int x){
	 // To send an SMS, call modem.sendSMS(SMS_TARGET, smsMessage)

	creatSMS(buffer_sms,x,0);
	return true;
}

uint8_t call_back_sms_loop_status_check(){
  return 0;
	/*return alarm_sms_index;*/
}

void call_back_alarm_snoozed(){
	 xEventGroupSetBits(EventRTOS_lcd,    TASK_1_BIT );
	/*Current_caller_state=0;
	alarm_calling_index=0;

	#ifdef GSM_OK
	gsm.SetCommLineStatus(CLS_FREE);
	

	if (CALL_ACTIVE_VOICE==call.CallStatus())

	{

		call.HangUp();

	}
	#endif*/
	
	
}

uint8_t call_back_alarm_bell_time_out(){
	Serial.println(F("Bell timeout"));
	digitalWrite(RELAY_ALARM,LOW);
	xEventGroupSetBits(EventRTOS_buzzer,    TASK_1_BIT );	
	char buffer2[50];
	sprintf(buffer2,"ALARM Muted in %d seconds!",systemConfig.bell_time_out);
	
	creatSMS(buffer2,1,0);	
	return 0;
}



void call_back_DISARM(uint8_t user, const char* _msg, eInvoking_source _last_invoker){
	/*systemConfig.last_system_state= DEACTIVE;
	eeprom_save();*/
	//Event log update;
	Timer_battery_charge.previousMillis = millis();
	digitalWrite(PIN_BATTERY,HIGH);
	//save_event_info(0,user,time_buffer,"DISARM");
	digitalWrite(PIN_ARM,LOW);
	digitalWrite(PIN_DISARM,HIGH);
	digitalWrite(RELAY_ALARM,LOW);
	xEventGroupSetBits(EventRTOS_buzzer,    TASK_6_BIT );
	char buffer[10];
	get_eInvoker_type_to_char(_last_invoker,buffer);
	
	
	char buffer2[25];
	sprintf(buffer2," user %d >%s",user,buffer);
	
	creat_disarm_sms(buffer2);
	//eCrrunt_lcd_page = DISARM;
	
	
	#ifdef GSM_OK
	
	/* if counter is equal 2 then set event bit of task1 */
	EventBits_t uxBits_gsm;

	/* Set bit 0 and bit 4 in xEventGroup. */
	uxBits_gsm = xEventGroupSetBits(EventRTOS_gsm,    TASK_2_BIT );
	xEventGroupClearBits(EventRTOS_gsm,    TASK_1_BIT );// call task invoking request
	
	// set event for lcd
	EventBits_t uxBits_lcd;
	
	uxBits_lcd = xEventGroupSetBits(EventRTOS_lcd,    TASK_2_BIT );
	#endif

	
}

void call_back_ARM(uint8_t user, const char* _msg, eInvoking_source _last_invoker){
	/*systemConfig.last_system_state= SYS1_IDEAL;
	eeprom_save();*/
	//Event log update;
	/*char time_buffer[25];
	gsm.getNetwork_time(time_buffer);*/
	
	//save_event_info(0,user,time_buffer,"ARM");
	
	
	digitalWrite(PIN_ARM,HIGH);
	digitalWrite(PIN_DISARM,LOW);
	xEventGroupSetBits(EventRTOS_buzzer,    TASK_5_BIT );
	char buffer[10];
	get_eInvoker_type_to_char(_last_invoker,buffer);
	char buffer2[25];
	sprintf(buffer2," user %d >%s",user,buffer);
	creat_arm_sms(buffer2);
	
	Current_caller_state=0;
	alarm_calling_index=1;	
	// set event for lcd
	EventBits_t uxBits_lcd;	
	uxBits_lcd = xEventGroupSetBits(EventRTOS_lcd,    TASK_1_BIT );
}

void call_back_system_is_not_ready(){
	
	/*Serial.print(F("System is not ready"));	
	 
	char temp_buffer[10];
	static uint8_t faulty_zone_count=0;
	memset(lcd_system_states,'\0',25);
	memset(lcd_faulty_sensors,'\0',25);
	for (uint8_t index=0;index<48;index++)
	{		
		if (!myAlarm_pannel.is_sensor_ready(index))
		{
			strcat(lcd_faulty_sensors,itoa(index,temp_buffer,10));
			strcat(lcd_faulty_sensors,",");
			faulty_zone_count++;		
		}		
		if (faulty_zone_count>8)
		{	//too many faulty zones			
			strcat(lcd_faulty_sensors,"*");
			break;
		}		
	}	
	
	if (faulty_zone_count<8)
	{		//only 8
		strcat(lcd_faulty_sensors,".");		
	}	
	
	
	char cmd[25];
	memset(cmd,'\0',25);
	
	sprintf(cmd,"NRZ=%s",lcd_faulty_sensors);
	sendCommand(cmd);
	bool ret = recvRetCommandFinished(500);
	if (!ret)
	{
		Serial.println(F("NRZ send fail"));
	}
	faulty_zone_count=0;
	#ifdef CAN_ENABLE
	system_not_ready_on_can();
	#endif	*/
	
}
void call_back_system_is_ready(){Serial.print(F("System is ready"));
	/*char cmd[25];
	memset(cmd,'\0',25);
	strcpy(cmd,"YRZ");
	Serial.println(cmd);
	sendCommand(cmd);
	bool ret = recvRetCommandFinished(500);
	if (!ret)
	{
		Serial.println(F("YRZ send fail"));
	}

	#ifdef CAN_ENABLE
	system_ready_on_can();
	#endif	*/
}

bool call_back_arm_faild(const char* srt ,int index){
	/*Serial.print(F("arm failed open zone found>"));
	Serial.println(F("waiting user selection"));


#ifdef CAN_ENABLE
	send_faulty_sens_infor_can();
	//stmp[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	uint8_t stmp[8];
	stmp[0] = ALM;
	CAN.sendMsgBuf(REQ_NUM_PARA_255, 0, 1, stmp);
	
#endif*/
return 0;
}

void call_back_arm_can_not_be_done(){
	/*
	#ifdef CAN_ENABLE
	//CAN
	system_userRequest_timeOut_broadcast_OnCAN();

	#endif*/
}


void call_back_Exit_delay_timer_start(){
	// set event for lcd
	Serial.println(">>>>>>>>>>>>>>>exit timer on");
	EventBits_t uxBits_lcd;
	
	uxBits_lcd = xEventGroupSetBits(EventRTOS_lcd,    TASK_4_BIT );
}

void call_back_Entry_delay_timer_start(){
	// set event for lcd
	EventBits_t uxBits_lcd;
	
	uxBits_lcd = xEventGroupSetBits(EventRTOS_lcd,    TASK_6_BIT );
	
}

void call_back_zone_state_update_notify(uint8_t zone0_7, uint8_t zone8_15, uint8_t zone16_23, uint8_t zone24_31, uint8_t zone32_39, uint8_t zone40_47){
	/*#ifdef CAN_ENABLE
	Serial.println(F("send zone update on can"));
		uint8_t stmp[8];
		stmp[0] = ZST;
		stmp[1]=zone0_7;
		stmp[2]=zone8_15;
		stmp[3]=zone16_23;
		stmp[4]=zone24_31;
		stmp[5]=zone32_39;
		stmp[6]=zone40_47;
		stmp[7]=0xff;
		//CAN.sendMsgBuf(BRODCAST_SYS_UPDATE, 0, 7, stmp);
		 broadcast_any_zone_state_update_on_can(BIT_MASK_LAST_STATE,SEND_ZONE_STATE_UPDATE,ZST);
		 broadcast_any_zone_state_update_on_can(BIT_MASK_BYPASSED,SEND_ZONE_STATE_UPDATE,ZBS);
	#endif*/
	// send update to MQTT
	
}

// chime alarm using i2c . zone state update on CAN bus will be sent separately. see >call_back_zone_state_update_notify();
bool call_back_zone_status_update(const char* srt ,int index){
	/*
		

		char msg[10]="zbuzz";

		sendOnSerial3_string(msg);

		
		
	#ifdef CAN_ENABLE

// 

#endif*/
	return 0;
	
}

bool call_back_Entry_delay_time_out(const char* srt ,int index){
	Serial.println(F("entry delay time out"));
	if (index!=-1)
	{
		Serial.println(F("pssword enter faild"));
		myAlarm_pannel.set_system_state(ALARM_CALLING,SYSTEM_ITSELF,0);
		
		// set event for lcd
		EventBits_t uxBits_lcd;
		
		uxBits_lcd = xEventGroupSetBits(EventRTOS_lcd,    TASK_3_BIT );
		
	}
	else{
		
		Serial.println(F("No problem"));
		myAlarm_pannel.set_system_state(DEACTIVE,SYSTEM_ITSELF,0);
		// set event for lcd
		EventBits_t uxBits_lcd;
		
		uxBits_lcd = xEventGroupSetBits(EventRTOS_lcd,    TASK_2_BIT );
		
	}
	
	return 0;
}

bool call_back_Exit_delay_time_out(const char* srt ,int index){
	
	
	
	if (index!=-1)
	{
		Serial.println(F("zone is still open"));	
		myAlarm_pannel.set_system_state(ALARM_CALLING,SYSTEM_ITSELF,0);
		// set event for lcd
		EventBits_t uxBits_lcd;
		
		uxBits_lcd = xEventGroupSetBits(EventRTOS_lcd,TASK_3_BIT );
		
	}
	else{	
		
		Serial.println(F("No problem"));
		myAlarm_pannel.set_system_state(SYS1_IDEAL,SYSTEM_ITSELF,0);
		//tun off buzzer and stay on arm state
		// set event for lcd
		EventBits_t uxBits_lcd;
		
		uxBits_lcd = xEventGroupSetBits(EventRTOS_lcd,TASK_1_BIT );
		
	}
	return 0;
}

void call_back_alarm_notify(uint8_t alarm_zone){
	//activate buzzer and alarm relay.
	xEventGroupSetBits(EventRTOS_buzzer,  TASK_2_BIT );
	digitalWrite(RELAY_ALARM,HIGH);

	char buffer_sms[25];
	sprintf(buffer_sms,"Alarm zone %s",get_device_name(alarm_zone));
	Serial.print("SMS creat>");
	Serial.println(buffer_sms);
	addTimeStamp(buffer_sms);
	creatSMS(buffer_sms,1,0);
	
	// send info to LCD
	char zone_char[25];
	memset(zone_char, '\0', 25);
	sprintf(zone_char,">%s",get_device_name(alarm_zone));
	const TickType_t x100ms = pdMS_TO_TICKS( 10 );
				/* Send the string to the message buffer.  Return immediately if there is
				not enough space in the buffer. */
				size_t xBytesSent;
				xBytesSent = xMessageBufferSend( xMessageBuffer_zone,
											( void * ) zone_char,
											strlen( zone_char ), 0 );

				if( xBytesSent != strlen( zone_char ) )
				{
					Serial.println(F("The string could not be added to the message buffer because there was not enough free space in the buffer"));
					/* . */
				}

}

void _call_back_rf_zone_re_enable(uint8_t zone){

#ifdef MQTT_OK
				send_sensor_state_update_to_mqtt(zone,0);		
#endif
	
}


char* get_device_name(uint8_t device_index){

	File fileToReadx;
	if(device_index < 8){
    fileToReadx = SPIFFS.open("/zone_data_8.json");
} else if(device_index >= 8 && device_index < 16){
    fileToReadx = SPIFFS.open("/zone_data_16.json");
} else if(device_index >= 16 && device_index < 24){
    fileToReadx = SPIFFS.open("/zone_data_24.json");
} else if(device_index >= 24 && device_index < 32){
    fileToReadx = SPIFFS.open("/zone_data_32.json");
} else if(device_index >= 32 && device_index < 40){
    fileToReadx = SPIFFS.open("/zone_data_40.json");
} else if(device_index >= 40 && device_index < 48){
    fileToReadx = SPIFFS.open("/zone_data_48.json");
}
else{
	return "invalid";
}
	
	
	DynamicJsonDocument docx(JSON_DOC_SIZE_ZONE_DATA);
	deserializeJson(docx,  fileToReadx);
	fileToReadx.close();
	char buff[20];
	
		memset(buff, '\0', 20);
		if (device_index < 10) {
			sprintf(buff, "zone0%d", device_index);
		}
		else {
			sprintf(buff, "zone%d", device_index);
		}
		
		const char *zone_name = docx[buff]["name"];
		
		strcpy(STRUCT_sens_infor.device_name,zone_name);
		
		return STRUCT_sens_infor.device_name;
		
	
}
void set_device_name(uint8_t device_index, const char* device_name){

	File fileToReadx;
	if(device_index < 8){
    fileToReadx = SPIFFS.open("/zone_data_8.json");
	} 
	else if(device_index >= 8 && device_index < 16){
		fileToReadx = SPIFFS.open("/zone_data_16.json");
	} 
	else if(device_index >= 16 && device_index < 24){
		fileToReadx = SPIFFS.open("/zone_data_24.json");
	} 
	else if(device_index >= 24 && device_index < 32){
		fileToReadx = SPIFFS.open("/zone_data_32.json");
	} 
	else if(device_index >= 32 && device_index < 40){
		fileToReadx = SPIFFS.open("/zone_data_40.json");
	} 
	else if(device_index >= 40 && device_index < 48){
		fileToReadx = SPIFFS.open("/zone_data_48.json");
	}

	 if(!fileToReadx){
		 Serial.println("? failed to open directory");
		 return;
	 }

	DynamicJsonDocument docx(JSON_DOC_SIZE_ZONE_DATA);

	deserializeJson(docx, fileToReadx);
	fileToReadx.close();
	
	/*serializeJsonPretty(docx, Serial);
	return;*/

	char buff[20];

	memset(buff, '\0', 20);
	if (device_index < 10) {
		sprintf(buff, "zone0%d", device_index);
	}
	else {
		sprintf(buff, "zone%d", device_index);
	}

	Serial.printf_P(PSTR("zone id:%d	zone name:%s\n"),device_index, device_name);

	docx[buff]["name"] = device_name;
	

	File fileToWritex;
	if(device_index < 8){
    fileToWritex = SPIFFS.open("/zone_data_8.json",FILE_WRITE);
	} 
	else if(device_index >= 8 && device_index < 16){
		fileToWritex = SPIFFS.open("/zone_data_16.json",FILE_WRITE);
	} 
	else if(device_index >= 16 && device_index < 24){
		fileToWritex = SPIFFS.open("/zone_data_24.json",FILE_WRITE);
	} 
	else if(device_index >= 24 && device_index < 32){
		fileToWritex = SPIFFS.open("/zone_data_32.json",FILE_WRITE);
	} 
	else if(device_index >= 32 && device_index < 40){
		fileToWritex = SPIFFS.open("/zone_data_40.json",FILE_WRITE);
	} 
	else if(device_index >= 40 && device_index < 48){
		fileToWritex = SPIFFS.open("/zone_data_48.json",FILE_WRITE);
	}
	serializeJson(docx, fileToWritex);
	fileToWritex.close();
	
	
}

int8_t comp_device_RFID(const char* rf_id_rx){
	int8_t ret = -1;

	File fileToReadx;

 for(uint8_t device_index = 0; device_index<48; device_index++){

	if(device_index < 8){
    fileToReadx = SPIFFS.open("/zone_data_8.json");
} else if(device_index >= 8 && device_index < 16){
    fileToReadx = SPIFFS.open("/zone_data_16.json");
} else if(device_index >= 16 && device_index < 24){
    fileToReadx = SPIFFS.open("/zone_data_24.json");
} else if(device_index >= 24 && device_index < 32){
    fileToReadx = SPIFFS.open("/zone_data_32.json");
} else if(device_index >= 32 && device_index < 40){
    fileToReadx = SPIFFS.open("/zone_data_40.json");
} else if(device_index >= 40 && device_index < 48){
    fileToReadx = SPIFFS.open("/zone_data_48.json");
}

	if(!fileToReadx){
		 Serial.println(F("? failed to open directory"));
		 return ret;
	 }

	DynamicJsonDocument docx(JSON_DOC_SIZE_ZONE_DATA);
	deserializeJson(docx,  fileToReadx);
	fileToReadx.close();
	char buff[20];
	

		memset(buff, '\0', 20);
		if (device_index < 10) {
			sprintf(buff, "zone0%d", device_index);
		}
		else {
			sprintf(buff, "zone%d", device_index);
		}
		//Serial.printf("get rfid for %s > %s",buff,rf_id_rx);
		const char* rf_id = docx[buff]["rfid"];
		//Serial.print(F("zone rfid is:"));
		//Serial.println(rf_id);

		if (strcmp(rf_id_rx, rf_id) == 0) {
			ret = device_index;
			break;
		}
 }
	
	return ret;
}

char* get_device_RFID(uint8_t device_index){
	File fileToReadx;
	if(device_index < 8){
    fileToReadx = SPIFFS.open("/zone_data_8.json");
	} 
	else if(device_index >= 8 && device_index < 16){
		fileToReadx = SPIFFS.open("/zone_data_16.json");
	} 
	else if(device_index >= 16 && device_index < 24){
		fileToReadx = SPIFFS.open("/zone_data_24.json");
	} 
	else if(device_index >= 24 && device_index < 32){
		fileToReadx = SPIFFS.open("/zone_data_32.json");
	} 
	else if(device_index >= 32 && device_index < 40){
		fileToReadx = SPIFFS.open("/zone_data_40.json");
	} 
	else if(device_index >= 40 && device_index < 48){
		fileToReadx = SPIFFS.open("/zone_data_48.json");
	}

	 if(!fileToReadx){
		 Serial.println("? failed to open directory");
		 return nullptr;
	 }
	
	DynamicJsonDocument docx(JSON_DOC_SIZE_ZONE_DATA);

	deserializeJson(docx,  fileToReadx);
	char buff[20];
	
	memset(buff, '\0', 20);
	
	
		if (device_index < 10) {
			sprintf(buff, "zone0%d", device_index);
		}
		else {
			sprintf(buff, "zone%d", device_index);
		}
		/*Serial.print("get rfid ");
		Serial.println(buff);*/
		const char* rf_id = docx[buff]["rfid"];	
	
	strcpy(STRUCT_sens_infor.device_rf_id, rf_id);
	
	return STRUCT_sens_infor.device_rf_id;
	
	
}

void set_device_RFID(uint8_t device_index, const char* rf_id){

	File fileToReadx;
	if(device_index < 8){
    fileToReadx = SPIFFS.open("/zone_data_8.json");
	} 
	else if(device_index >= 8 && device_index < 16){
		fileToReadx = SPIFFS.open("/zone_data_16.json");
	} 
	else if(device_index >= 16 && device_index < 24){
		fileToReadx = SPIFFS.open("/zone_data_24.json");
	} 
	else if(device_index >= 24 && device_index < 32){
		fileToReadx = SPIFFS.open("/zone_data_32.json");
	} 
	else if(device_index >= 32 && device_index < 40){
		fileToReadx = SPIFFS.open("/zone_data_40.json");
	} 
	else if(device_index >= 40 && device_index < 48){
		fileToReadx = SPIFFS.open("/zone_data_48.json");
	}

	 if(!fileToReadx){
		 Serial.println("? failed to open directory");
		 return;
	 }
	
	 if(!fileToReadx){
		 Serial.println(F("? failed to open directory"));
		 return;
	 }

	DynamicJsonDocument docx(JSON_DOC_SIZE_ZONE_DATA);

	deserializeJson(docx, fileToReadx);
	fileToReadx.close();
	
	/*serializeJsonPretty(docx, Serial);
	return;*/

	char buff[20];

	memset(buff, '\0', 20);
	if (device_index < 10) {
		sprintf(buff, "zone0%d", device_index);
	}
	else {
		sprintf(buff, "zone%d", device_index);
	}

	Serial.printf_P(PSTR("set zone rfid:%s"),buff);
	docx[buff]["rfid"] = rf_id;	
	
	File fileToWritex;
	if(device_index < 8){
    fileToWritex = SPIFFS.open("/zone_data_8.json",FILE_WRITE);
	} 
	else if(device_index >= 8 && device_index < 16){
		fileToWritex = SPIFFS.open("/zone_data_16.json",FILE_WRITE);
	} 
	else if(device_index >= 16 && device_index < 24){
		fileToWritex = SPIFFS.open("/zone_data_24.json",FILE_WRITE);
	} 
	else if(device_index >= 24 && device_index < 32){
		fileToWritex = SPIFFS.open("/zone_data_32.json",FILE_WRITE);
	} 
	else if(device_index >= 32 && device_index < 40){
		fileToWritex = SPIFFS.open("/zone_data_40.json",FILE_WRITE);
	} 
	else if(device_index >= 40 && device_index < 48){
		fileToWritex = SPIFFS.open("/zone_data_48.json",FILE_WRITE);
	}
	serializeJson(docx, fileToWritex);
	fileToWritex.close();
	
}

int8_t comp_User_id(const char *number){
	
	int8_t ret = -1;
	
	File fileToReadx = SPIFFS.open("/personx.json");
	if(!fileToReadx){
		Serial.println(F("? failed to open directory"));
		return ret;
	}
	DynamicJsonDocument docx(JSON_DOC_SIZE_USER_DATA);
	deserializeJson(docx,  fileToReadx);
	char buff[20];
	for (int user_index = 1; user_index < 9; user_index++)
	{
		memset(buff, '\0', 20);
		sprintf(buff, "P%d", user_index);
		const char* user_number_strord = docx[buff]["number"];
		//if (user_number_strord!=NULL)
		if (phone_number_validat(user_number_strord))
		{			
			if (strncmp(number, user_number_strord, 12) == 0) {//+94714427691
				ret = user_index;
				break;
			}
		}
		
	}
	return ret;
	
}

int8_t comp_remote_RFID(uint32_t rx_rf_id_uint, uint8_t command_length){
	int8_t ret = -1;
	char stored_rf_id_char[25];
	memset(stored_rf_id_char,'\0', 25);
	
	File fileToReadx = SPIFFS.open("/personx.json");
	 if(!fileToReadx){
		 Serial.println(F("? failed to open directory"));
		 return ret;
	}	
	DynamicJsonDocument docx(JSON_DOC_SIZE_USER_DATA);
	deserializeJson(docx,  fileToReadx);
	char buff[20];
	for (int user_index = 1; user_index < 9; user_index++)
	{
		memset(buff, '\0', 20);		
	    sprintf(buff, "P%d", user_index);
		const char* remRf_id = docx[buff]["remID"];
		Serial.println(remRf_id);
			if (remRf_id!=NULL)
			{
				uint32_t stored_rf_id_uint = strtoul(remRf_id, NULL, 10);
				uint32_t split_rem_command = stored_rf_id_uint >> command_length;
				
				if (split_rem_command == rx_rf_id_uint) {
					ret = user_index;
					break;
				}
			}
		
	}
		return ret;	
}

char* get_remote_RFID(uint8_t device_index){
	
	File fileToReadx = SPIFFS.open("/personx.json");	
	DynamicJsonDocument docx(JSON_DOC_SIZE_USER_DATA);
	deserializeJson(docx,  fileToReadx);
	char buff[20];	
	memset(buff, '\0', 20);
	sprintf(buff, "P%d", device_index);	
	const char *remote_rfid = docx[buff]["remID"];	
	if (remote_rfid!=NULL)
	{
		strcpy(STRUCT_user_remote_infor.remote_rf_id,remote_rfid);
	}
	else{
		strcpy(STRUCT_user_remote_infor.remote_rf_id,"-1");
	}
	return STRUCT_user_remote_infor.remote_rf_id;	
}

void set_remote_RFID(uint8_t device_index, const char* rf_id){
	
	DynamicJsonDocument docx(JSON_DOC_SIZE_DEVICE_DATA);

	File fileToRead = SPIFFS.open("/personx.json");
	 if(!fileToRead){
		 Serial.println(F("? failed to open directory"));
		 return;
	 }

	deserializeJson(docx, fileToRead);
	fileToRead.close();
	
	char buff[20];

	memset(buff, '\0', 20);
	
		sprintf(buff, "P%d", device_index);
	


	Serial.print(F("set remote id"));
	Serial.println(buff);
	docx[buff]["remID"] = rf_id;	
	
	File fileToWritex = SPIFFS.open("/personx.json",FILE_WRITE);
	if(!fileToWritex){
		Serial.println(F("? failed to open directory"));
		return;
	}
	serializeJson(docx, fileToWritex);
	fileToWritex.close();
	
	//serializeJsonPretty(docx, Serial);
}

byte set_zone_param(const char* smsbuffer){
	 //char smsbuffer[] = "Zone=03,EXIT,0";
		 char buffer[50] = {0};
		 uint8_t device_index;
		 char *token;		 
		 bool param_state;
		 strcpy(buffer,smsbuffer);		  
		 token = strtok(buffer, "=,");
		 token = strtok(NULL, "=,");
		 device_index = atoi(token);
		
	File fileToReadx;
	
	if(device_index>=0 && device_index < 8){
    fileToReadx = SPIFFS.open("/zone_data_8.json");
	} 
	else if(device_index >= 8 && device_index < 16){
		fileToReadx = SPIFFS.open("/zone_data_16.json");
	} 
	else if(device_index >= 16 && device_index < 24){
		fileToReadx = SPIFFS.open("/zone_data_24.json");
	} 
	else if(device_index >= 24 && device_index < 32){
		fileToReadx = SPIFFS.open("/zone_data_32.json");
	} 
	else if(device_index >= 32 && device_index < 40){
		fileToReadx = SPIFFS.open("/zone_data_40.json");
	} 
	else if(device_index >= 40 && device_index < 48){
		fileToReadx = SPIFFS.open("/zone_data_48.json");
	}
	else{
	
		 return 0;
	}
	
	 if(!fileToReadx){
		 Serial.println(F("? failed to open directory"));
		 
		 return 0;
	 }

	DynamicJsonDocument docx(JSON_DOC_SIZE_ZONE_DATA);

	deserializeJson(docx, fileToReadx);
	fileToReadx.close();

		
		 char zone_index_char[15] = {0};

		 if (device_index>9)
		 {
			 sprintf(zone_index_char,"zone%d",device_index);
		 }
		 else{
			 sprintf(zone_index_char,"zone0%d",device_index);
		 }
		 char zone_paraName_char[20];
		 memset(zone_paraName_char,'\0',20);
		 token = strtok(NULL, "=,");		 
		 if (strcmp_P(token,PSTR("ENTRY"))==0){strcpy(zone_paraName_char,"entry_delay"); Serial.println(F("entry_delay"));}
		 else if (strcmp_P(token,PSTR("EXIT"))==0){strcpy(zone_paraName_char,"exit_delay");Serial.println(F("exit_delay"));}
		 else if (strcmp_P(token,PSTR("24H"))==0){strcpy(zone_paraName_char,"x24h");Serial.println(F("x24h"));}
		 else if (strcmp_P(token,PSTR("CHIME"))==0){strcpy(zone_paraName_char,"silent");Serial.println(F("silent"));}
		 else if (strcmp_P(token,PSTR("BYPASS"))==0){strcpy(zone_paraName_char,"bypass");Serial.println(F("bypass"));}
		 else {
			Serial.println(F("ERROR"));
			return 0;
		 }		 

		 token = strtok(NULL, "=,");
		 param_state = (token[0] == '1');
		docx[zone_index_char][zone_paraName_char]= param_state;		
		Serial.printf_P(PSTR("zone id:%d	para:%s	state:%d"),device_index,zone_paraName_char,param_state);
 	
	 File fileToWritex;
		if(device_index>=0 && device_index < 8){
			fileToWritex = SPIFFS.open("/zone_data_8.json",FILE_WRITE);
		} 
		else if(device_index >= 8 && device_index < 16){
			fileToWritex = SPIFFS.open("/zone_data_16.json",FILE_WRITE);
		} 
		else if(device_index >= 16 && device_index < 24){
			fileToWritex = SPIFFS.open("/zone_data_24.json",FILE_WRITE);
		} 
		else if(device_index >= 24 && device_index < 32){
			fileToWritex = SPIFFS.open("/zone_data_32.json",FILE_WRITE);
		} 
		else if(device_index >= 32 && device_index < 40){
			fileToWritex = SPIFFS.open("/zone_data_40.json",FILE_WRITE);
		} 
		else if(device_index >= 40 && device_index < 48){
			fileToWritex = SPIFFS.open("/zone_data_48.json",FILE_WRITE);
		}
		serializeJson(docx, fileToWritex);
		fileToWritex.close();
		return 1;
		
}

char* get_GSM_number(uint8_t gsm_number_index){// strat from 
		File fileToReadx = SPIFFS.open("/personx.json");
		
		DynamicJsonDocument docx(2048);
		DeserializationError err = deserializeJson(docx,  fileToReadx);
		if (err) {
			Serial.print(F("deserializeJson() failed: "));
			Serial.println(err.c_str());
		}
		
		char buff[20];		
		memset(buff, '\0', 20);		
		sprintf(buff, "P%d", gsm_number_index);		
		const char *gsm_number = docx[buff]["number"];	
		
		if (gsm_number!=nullptr)
		{
			strcpy(STRUCT_GSM_contact_infor.number,gsm_number);
		}
		else{
			strcpy(STRUCT_GSM_contact_infor.number,"-1");
		}
		return STRUCT_GSM_contact_infor.number;
	
	
}

void set_GSM_number(uint8_t gsm_number_index, const char* gsm_number){
	
	DynamicJsonDocument docx(JSON_DOC_SIZE_DEVICE_DATA);
	File fileToRead = SPIFFS.open("/personx.json");
	 if(!fileToRead){
		 Serial.println(F("? failed to open directory"));
		 return;
	 }
	deserializeJson(docx, fileToRead);
	fileToRead.close();	
	char buff[20];
	memset(buff, '\0', 20);
	
		sprintf(buff, "P%d", gsm_number_index);
		docx[buff]["number"] = gsm_number;		
	File fileToWritex = SPIFFS.open("/personx.json",FILE_WRITE);
	if(!fileToWritex){
		Serial.println(F("? failed to open directory"));
		return;
	}
	serializeJson(docx, fileToWritex);
	fileToWritex.close();
}

bool get_is_GSM_number_call(uint8_t gsm_number_index){
	//Serial.print("millis A");
	//Serial.println(millis());
	File fileToReadx = SPIFFS.open("/personx.json");
	DynamicJsonDocument docx(2048);
	DeserializationError err = deserializeJson(docx,  fileToReadx);
	if (err) {
		Serial.print(F("deserializeJson() failed: "));
		Serial.println(err.c_str());
		
	}
	
	char buff[20];
	memset(buff, '\0', 20);
	sprintf(buff, "P%d", gsm_number_index);
	bool call_en = docx[buff]["call"];
	//Serial.print("millis B");
	//Serial.println(millis());
	return call_en;
	
}

void set_GSM_number_is_call(uint8_t gsm_number_index, bool call_en){
	
	/*int ee_address = ADDR_OFFSET_EEPROM_GSM_NUMBERS + (sizeof(GSM_CONTACTS_INFO))*gsm_number_index;
	EEPROM.get(ee_address,STRUCT_GSM_contact_infor);
	STRUCT_GSM_contact_infor.call_num=call_en;
	EEPROM.put(ee_address,STRUCT_GSM_contact_infor);*/
}

bool get_is_GSM_number_sms(uint8_t gsm_number_index){
	//Serial.print("millis A");
	//Serial.println(millis());
	File fileToReadx = SPIFFS.open("/personx.json");
	DynamicJsonDocument docx(2048);
	DeserializationError err = deserializeJson(docx,  fileToReadx);
	if (err) {
		Serial.print(F("deserializeJson() failed: "));
		Serial.println(err.c_str());
		
	}
	
	char buff[20];
	memset(buff, '\0', 20);
	sprintf(buff, "P%d", gsm_number_index);
	bool sms_en = docx[buff]["sms"];
	//Serial.print("millis B");
	//Serial.println(millis());
	return sms_en;
	
}

void set_GSM_number_security_level(uint8_t gsm_number_index, uint8_t security_level){
	
	/*int ee_address = ADDR_OFFSET_EEPROM_GSM_NUMBERS + (sizeof(GSM_CONTACTS_INFO))*gsm_number_index;
	EEPROM.get(ee_address,STRUCT_GSM_contact_infor);
	STRUCT_GSM_contact_infor.call_num=security_level;
	EEPROM.put(ee_address,STRUCT_GSM_contact_infor);*/
}

uint8_t get_GSM_number_security_level(uint8_t gsm_number_index){
	
	/*int ee_address = ADDR_OFFSET_EEPROM_GSM_NUMBERS + (sizeof(GSM_CONTACTS_INFO))*gsm_number_index;
	EEPROM.get(ee_address,STRUCT_GSM_contact_infor);
	//Serial.println(STRUCT_user_remote_infor.remote_rf_id);
	return STRUCT_GSM_contact_infor.security_level;*/
	return 0;
	
}

void set_GSM_number_is_sms(uint8_t gsm_number_index, bool sms_en){
	
	/*int ee_address = ADDR_OFFSET_EEPROM_GSM_NUMBERS + (sizeof(GSM_CONTACTS_INFO))*gsm_number_index;
	EEPROM.get(ee_address,STRUCT_GSM_contact_infor);
	STRUCT_GSM_contact_infor.sms_num=sms_en;
	EEPROM.put(ee_address,STRUCT_GSM_contact_infor);*/
}

void get_EVENT_infor(uint8_t event_index, char*event_buffer, uint8_t len){
	
	
	/*int ee_address = ADDR_OFFSET_EEPROM_EVENTS + (sizeof(EVENT_INFO))*event_index;	
	EEPROM.get(ee_address,STRUCT_event_infor);
	char buffer[25];
	memset(buffer,'\0',25);
	sprintf(event_buffer,"%d-%d %d:%d",STRUCT_event_infor.tm_mon,STRUCT_event_infor.tm_day,STRUCT_event_infor.tm_hour,STRUCT_event_infor.tm_min);
	
	strcat(event_buffer,STRUCT_event_infor.user_id);
	strcat(event_buffer,STRUCT_event_infor.discription);
	Serial.print(F("Event>"));
	Serial.println(event_buffer);*/
	
	
}

void set_EVENT_infor(uint8_t event_index, char* discription){
	
}

void save_event_info(uint8_t event_type_id, uint8_t user_id, char* event_time, char* event_discriptio){
	
}



void setup_call_backs(){

	myAlarm_pannel.set_fn_arm(call_back_ARM);
	myAlarm_pannel.set_fn_zone_state_update_notify(call_back_zone_state_update_notify);
	myAlarm_pannel.set_fn_alarm_calling(call_back_alarm_Calling);
	
	
	myAlarm_pannel.set_fn_creat_alarm_msg(call_back_creat_alarm_sms);// creat any msg
	
	myAlarm_pannel.set_fn_alarm_notify(call_back_alarm_notify);// alarm notify
	
	myAlarm_pannel.set_fn_disarm(call_back_DISARM);
	myAlarm_pannel.set_fn_alarm_snooze(call_back_alarm_snoozed);
	myAlarm_pannel.set_fn_alarm_bell_time_out(call_back_alarm_bell_time_out);
	myAlarm_pannel.set_fn_rf_zone_re_enable(_call_back_rf_zone_re_enable);
	myAlarm_pannel.set_fn_entry_delay_timer_start(call_back_Entry_delay_timer_start);
	myAlarm_pannel.set_fn_chime_sound(call_back_chime_sound);
	myAlarm_pannel.set_fn_exit_delay_timer_start(call_back_Exit_delay_timer_start);
	myAlarm_pannel.set_fn_entry_delay_time_out(call_back_Entry_delay_time_out);
	myAlarm_pannel.set_fn_exit_delay_time_out(call_back_Exit_delay_time_out);
	myAlarm_pannel.set_call_back_sms_loop_status(call_back_sms_loop_status_check);
	myAlarm_pannel.set_fn_arm_faild(call_back_arm_faild);
	myAlarm_pannel.set_fn_zone_status_update(call_back_zone_status_update);
	myAlarm_pannel.set_fn_system_is_not_ready(call_back_system_is_not_ready);
	myAlarm_pannel.set_fn_system_is_ready(call_back_system_is_ready);
	myAlarm_pannel.set_fn_arm_can_not_be_done(call_back_arm_can_not_be_done);
	myAlarm_pannel. set_fn_save_event_info(save_event_info);
	
	myAlarm_pannel.set_entry_delay_timer_interval(10);
	myAlarm_pannel.set_exit_delay_timer_interval(15);
	myAlarm_pannel.set_bell_time_timer_interval(systemConfig.bell_time_out);
	
	myAlarm_pannel.set_fn_set_name(set_device_name);
	myAlarm_pannel.set_fn_get_name(get_device_name);
	
	myAlarm_pannel.set_fn_set_rfid(set_device_RFID);
	myAlarm_pannel.set_fn_get_rfid(comp_device_RFID);
	
	myAlarm_pannel.set_fn_set_remote_id(set_remote_RFID);
	myAlarm_pannel.set_fn_get_remote_id(get_remote_RFID);
	
	myAlarm_pannel.set_fn_intializ_sensors(initiliz_sensor_data);
#ifdef MQTT_OK
	set_onMQTT_connection(onMqtt_connection);
#endif
}

void onMqtt_connection(){
	for(uint8_t index = 0; index<4; index++){
		send_sensor_state_update_to_mqtt(index,digitalRead(GPIO_array[index].GPIOpin));
	}
	Serial.println(F("send zone states to mqtt"));	
	
	if(myAlarm_pannel.get_system_state()!=DEACTIVE){
		 publish_system_state("ARMED","info/mode",true);
		
	}
	else{
		publish_system_state("DISARMED","info/mode",true);
	}

}

void call_back_chime_sound(){
	xEventGroupSetBits(EventRTOS_buzzer,    TASK_3_BIT );
}