#include "universalEventx.h"

char netowrk_operator_name_char[10];

byte universal_event_hadler(const char* smsbuffer, eInvoking_source Invoker, uint8_t user_id){//0- gsm 1-lcd
	byte ret_value = 0;
	Serial.print(F("Run CLI.. _cliString>"));
	Serial.print(smsbuffer);
	Serial.print(F(" Access level: "));
	Serial.println(systemConfig.cli_access_level);
	Serial.print(F(" User: "));
	Serial.println(user_id);
	Serial.print(F(" Invoker: "));
	
	switch (Invoker)
	{
		case GSM_MODULE:Serial.println(F("GSM"));
		break;
		case PSERIAL0:Serial.println(F("SERIAL0"));
		break;
		case LCD:Serial.println(F("LCD"));
		break;
		case CARD_A:Serial.println(F("CARD A"));
		break;
		case CARD_B:Serial.println(F("CARD B"));
		break;
		case CARD_C:Serial.println(F("CARD C"));
		break;
		case APP:Serial.println(F("App"));
		break;
	}
	
	if (strncmp("log",smsbuffer,3)==0)
	{
		Serial.println(F("Reading Log"));
        delay(500);
		//processOfflineMessagesV2();
		readLastNMessagesToQueue(10);
		processMessagesFromQueue();
		Serial.println("loop2 exit");
		
	}
	if (strncmp("reboot",smsbuffer,6)==0)
	{
		Serial.println(F("Restart in 5 sec"));
        delay(5000);
		ESP.restart();
	}
    if (strncmp("sms ap",smsbuffer,6)==0)
	{
        if(setJson_key_bool("/config.json", "wifiap_en", true)){
            Serial.println(F("AP setup ok"));
        }
		Serial.println(F("Restart in 5 sec"));        
        delay(5000);
		ESP.restart();
	}
	
	if (strncmp("net?",smsbuffer,4)==0)
	{
		
		Serial.print(F("Network>"));
		Serial.println(netowrk_operator_name_char);
	}
	if (strncmp("info?",smsbuffer,5)==0)
	{
		
		
	}
	if (strncmp("BLOCK",smsbuffer,5)==0)
	{
		while(1){}
		
	}
	
	
	if (strncmp("psw=1234",smsbuffer,3)==0)
	{
		ret_value = 1;
		char psw_char[6];
		strlcpy(psw_char,smsbuffer+4,5);
		int psw_int = atoi(psw_char);
		Serial.println(F("password>"));
		Serial.println(psw_int);
		if (psw_int==1234)
		{
			systemConfig.cli_access_level=3;
			Serial.println(F("cli unlocked"));
		}
		else{
			systemConfig.cli_access_level=0;
			Serial.println(F("cli locked"));
		}
	}
	if (strncmp_P(smsbuffer,PSTR("Name="),4)==0)// Name=01,Front D
	{
		ret_value = 1;
		//check access
		//if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3);}
		//Serial.print(F("zone id>> "));
		char zone_index[5]={0};
		char zone_name[10]={0};
		strlcpy	(zone_index,smsbuffer+5,3);
		uint8_t zone_index_int = atoi(zone_index);		
		strcpy	(zone_name,smsbuffer+8);		
		set_device_name(zone_index_int, zone_name);
		Serial.printf_P(PSTR("**zone renaming**\n"));
		if (Invoker==GSM_MODULE)
		{
			char buffer[5];
			strcpy_P(buffer,PSTR("OK"));
			creatSMS(buffer,2,0);
		}
	}	
	
	if (strncmp_P(smsbuffer,PSTR("Zone="),4)==0)// "Zone=03,EXIT,0";
	{
		ret_value = 1;
		 set_zone_param(smsbuffer);
		 eeprom_load(0);
		 char reply_buff[5];
		 strcpy_P(reply_buff,PSTR("OK"));
		 creatSMS(reply_buff,2,0);
	}
	if (strncmp("Status?",smsbuffer,7)==0)
	{
		ret_value = 1;
		char msg[150] = {0};
		String ipAddress = WiFi.localIP().toString();
  		String macAddress = WiFi.macAddress();
  		int32_t rssi = WiFi.RSSI();
		String wifiStatus;
		(!WiFi.isConnected())?wifiStatus = "offline": wifiStatus = "online";
  		sprintf(msg, "WiFi:%s\nSSID:%s\nPW:%s\nIP:%s\nMAC:%s\nRSSI:%d dBm\n",wifiStatus,systemConfig.wifissid_sta,systemConfig.wifipass, ipAddress.c_str(), macAddress.c_str(), rssi);
		//sms_broad_cast_request = true;
		SMS_to_be_sent_FIXDMEM[sms_buffer_msg_count].type = 1;
		if (myAlarm_pannel.get_system_state()!=DEACTIVE)
		{
			strcat_P(msg,PSTR("Mode: ARMED"));
			creatSMS(msg,3,0);
		}
		else
		{
			strcat_P(msg,PSTR("Mode: DISARMED"));
			creatSMS(msg,3,0);
		}
		Serial.println(msg);
		
	}
	if (strncmp_P(smsbuffer,PSTR("Home arm"),8)==0)
	{
		ret_value = 1;
		myAlarm_pannel.set_arm_mode(AS_ITIS_NO_BYPASS);
		myAlarm_pannel.set_system_state(SYS1_IDEAL,Invoker,user_id);
#ifdef MQTT_OK
		publish_system_state("ARMED","info/mode",true);
#endif       
		
	}
	if (strncmp_P(smsbuffer,PSTR("Home arm"),8)==0)
	{
		ret_value = 1;
		myAlarm_pannel.set_arm_mode(AS_ITIS_NO_BYPASS);
		myAlarm_pannel.set_system_state(SYS1_IDEAL,Invoker,user_id);
#ifdef MQTT_OK
		publish_system_state("ARMED","info/mode",true);
#endif       
		
	}
	
	else if(strncmp_P(smsbuffer,PSTR("Disarm"),6)==0){
		ret_value = 1;	
		eCurrent_state=DEACTIVE;
		myAlarm_pannel.set_system_state(DEACTIVE,Invoker,user_id);
#ifdef MQTT_OK
		publish_system_state("DISARMED","info/mode",true);
#endif        
		
	}
	
	else if(strncmp("Panic",smsbuffer,5)==0)
	{
		ret_value =1;
		myAlarm_pannel.set_system_state(PANIC,Invoker,user_id);
	}

	else if(strncmp("Alarm_call",smsbuffer,10)==0)
	{
		ret_value =1;
		//myAlarm_pannel.set_system_state(PANIC,Invoker,user_id);
		myAlarm_pannel.set_system_state(ALARM_CALLING,WEB,0);
#ifdef MQTT_OK
		publish_system_state("ALARM","info/mode",true);
#endif   
	}
	
	else if(strncmp("Number",smsbuffer,6)==0)//Number=1,+94714427691
	{
		ret_value = 1;
		//check access
		//if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3); }
		
		 char number[25];
		 memset(number,'\0',25);
		 strcpy(number,smsbuffer+7);
		 char *index_str = strtok(number, ",");
		 if (index_str == NULL) {
			 Serial.println(F("Error: Invalid input string."));
			 ret_value = 0;
			 return ret_value;
			
		 }

		 char *phone_num_str = strtok(NULL, ",");
		 if (phone_num_str == NULL) {
			 Serial.println(F("Error: Invalid input string."));
			  ret_value = 0;
			  return ret_value;
		 }

		 int tp_number_index = atoi(index_str);
		 if (tp_number_index == 0 && index_str[0] != '0') {
			 Serial.println(F("Error: Invalid zone_index_int value."));
			  ret_value = 0;
			  return ret_value;
		 }

		 Serial.print(F("Index:"));
		 Serial.println(tp_number_index);
		 Serial.print(F("Phone number:"));
		 Serial.println(phone_num_str);
		 char reply_sms[159];
		 memset(reply_sms,'\0',sizeof(reply_sms));
		 char phone_number_char[13];
		 memset(phone_number_char,0, sizeof(phone_number_char));
		 strlcpy(phone_number_char,phone_num_str,13);
		if (phone_number_validat(phone_number_char))
		{
			set_GSM_number(tp_number_index, phone_number_char);
			strcat	(reply_sms,"Numbers");
			strcat	(reply_sms,"\n");
			strcat	(reply_sms,get_GSM_number(1));
			strcat	(reply_sms,"\n");
			strcat	(reply_sms,get_GSM_number(2));
			strcat	(reply_sms,"\n");
			strcat	(reply_sms,get_GSM_number(3));
			strcat	(reply_sms,"\n");
			strcat	(reply_sms,get_GSM_number(4));
			strcat	(reply_sms,"\n");
			strcat	(reply_sms,get_GSM_number(5));
			strcat	(reply_sms,"\n");
			strcat	(reply_sms,get_GSM_number(6));
			strcat	(reply_sms,"\n");
			strcat	(reply_sms,get_GSM_number(7));
			strcat	(reply_sms,"\n");
			strcat	(reply_sms,get_GSM_number(8));
			
		} 
		else
		{
			strcat_P(reply_sms,PSTR("Invalide number format. Eg:+94712345890"));
		}
		
		creatSMS(reply_sms,1,0);
		
		
		
		
		//sms_broad_cast_request=true;
	}
	else if(strncmp_P(smsbuffer,PSTR("SMS number="),10)==0){// set sms number eg: Sms number=01,1
		ret_value = 1;
		//check access
		if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3,0);}
		
		char* eq_start = strstr(smsbuffer,"=");
		char zone_index[10];
		char zone_name[5];
		
		strlcpy(zone_index,eq_start+1,3);
		int tp_number_index = atoi(zone_index);
		Serial.print(F("TP number:"));
		Serial.print(zone_index);
		strlcpy(zone_name,eq_start+4,2);
		int state = atoi(zone_name);
		if (state)
		{
			set_GSM_number_is_sms(tp_number_index,true);
			Serial.println(F("sms enable"));
		}
		else
		{
			set_GSM_number_is_sms(tp_number_index,false);
			Serial.println(F("sms disable"));
		}
		
		eeprom_save();
		if (Invoker==GSM_MODULE)
		{
			char buffer[5];
			strcpy_P(buffer,PSTR("OK"));
			creatSMS(buffer,2,0);
		}
	}
	else if(strncmp_P(smsbuffer,PSTR("CALL number="),11)==0){// set call number
		ret_value = 1;
		//check access
		if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3,0); }
		
		char* eq_start = strstr(smsbuffer,"=");
		char zone_index[10];
		char zone_name[5];
		
		strlcpy(zone_index,eq_start+1,3);
		int tp_number_index = atoi(zone_index);
		Serial.print(F("TP number:"));
		Serial.print(zone_index);
		Serial.print(F("="));
		strlcpy(zone_name,eq_start+4,2);
		int state = atoi(zone_name);
		eeprom_save();
		if (state)
		{
			set_GSM_number_is_call(tp_number_index,true);
			Serial.println(F("call enable"));
		}
		else
		{
			set_GSM_number_is_call(tp_number_index,false);
			Serial.println(F("call disable"));
		}
		if (Invoker==GSM_MODULE)
		{
			char buffer[5];
			strcpy_P(buffer,PSTR("OK"));
			creatSMS(buffer,2,0);
		}
	}
	
	
	else if(strncmp("eerstm2",smsbuffer,7)==0){//remote B
		
		
	}
	else if (strncmp("RID=", smsbuffer, 4) == 0) {//remote B
	ret_value = 1;
	//RFbaster(smsbuffer);
	//send_rfid_state_update_to_mqtt(smsbuffer);
	}
	
	else if(strncmp("eerstm1",smsbuffer,7)==0){//remote B
		ret_value = 1;
		//check access
		//if (sys_data.cli_access_level<2){ creatSMS("Unauthorized action",3); return; }
		Serial.println(F("EEPROM Reset"));
		configReset(); configLoad(0);
		
	}
	
	else if (strncmp("infor=",smsbuffer,6)==0)//request infor
	{
		
		
	}	
	else if (strncmp_P(smsbuffer,PSTR("Entry delay="),12)==0)
	{
		ret_value = 1;
		//check access
		if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3,0); }
		
		char zone_index[10];
		
		strlcpy(zone_index,smsbuffer+12,3);
		int entry_delay_int = atoi(zone_index);
		systemConfig.entry_delay_time = entry_delay_int;
		Serial.print("Entry delay:");
		Serial.println(entry_delay_int);
		myAlarm_pannel.set_entry_delay_timer_interval(entry_delay_int);
		eeprom_save();
		if (Invoker==GSM_MODULE)
		{
			char buffer[5];
			strcpy_P(buffer,PSTR("OK"));
			creatSMS(buffer,2,0);
		}
	}
	else if (strncmp(smsbuffer,PSTR("Exit delay="),11)==0)
	{
		ret_value = 1;
		//check access
		if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3,0); }
		
		char zone_index[10];
		strlcpy(zone_index,smsbuffer+11,3);
		int exit_delay_int = atoi(zone_index);
		systemConfig.exit_delay_time = exit_delay_int;
		Serial.print(F("Exit delay:"));
		Serial.println(exit_delay_int);
		myAlarm_pannel.set_exit_delay_timer_interval(exit_delay_int);
		eeprom_save();
		if (Invoker==GSM_MODULE)
		{
			char buffer[5];
			strcpy_P(buffer,PSTR("OK"));
			creatSMS(buffer,2,0);
		}
	}
	
	else if (strncmp("sc=",smsbuffer,3)==0)// sensor state update sc=02,1 : zone 2 opend
	{
		char zone_index[10];
		char zone_name[10];
		strlcpy(zone_index,smsbuffer+3,3);
		strlcpy(zone_name,smsbuffer+6,2);
		int zone = atoi(zone_index);
		int state = atoi(zone_name);
		myAlarm_pannel.Universal_zone_state_update(zone,state);
		//send update to MQTT
#ifdef MQTT_OK
		send_sensor_state_update_to_mqtt(zone,state);		
#endif		
	}

	else if (strncmp("Relay 2 off",smsbuffer,10)==0)
	{
		ret_value = 1;
		//check access
		//if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3,0); }
		//Relay_A.Activate(2000);
		char buffer[25];
		strcpy_P(buffer,PSTR("Relay 2 off OK"));
		creatSMS(buffer,2,0);
#ifdef MQTT_OK
		publish_system_state("off","cmd/relay2/status", true);
#endif
#ifdef GSM_MINI_BOARD_V3
		digitalWrite(RELAY_OUT_B,HIGH);
#endif
	}
	else if (strncmp("Relay 2 on",smsbuffer,10)==0)
	{
		ret_value = 1;
		//check access
		//if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3,0); }
		//Relay_A.Activate(2000);
		char buffer[25];
		strcpy_P(buffer,PSTR("Relay 2 on OK"));
		creatSMS(buffer,2,0);
#ifdef MQTT_OK
		publish_system_state("on","cmd/relay2/status", true);
#endif
#ifdef GSM_MINI_BOARD_V3
		digitalWrite(RELAY_OUT_B,LOW);
#endif
	}
	else if (strncmp("Relay 1 on",smsbuffer,10)==0)
	{
		ret_value = 1;
		//check access
		//if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3,0); }
		//relay_B_activate();
		char buffer[25];
		strcpy_P(buffer,PSTR("Relay 1 on OK"));
		creatSMS(buffer,2,0);
#ifdef MQTT_OK
		publish_system_state("on","cmd/relay1/status", true);
#endif
#ifdef GSM_MINI_BOARD_V3
		digitalWrite(RELAY_OUT_A,LOW);
#endif
	}
	else if (strncmp("Relay 1 off",smsbuffer,10)==0)
	{
		ret_value = 1;
		//check access
		//if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3,0); }
		//relay_B_deactivate();char buffer[5];
		char buffer[25];
		strcpy_P(buffer,PSTR("Relay 1 off OK"));
		creatSMS(buffer,2,0);
#ifdef MQTT_OK
		publish_system_state("off","cmd/relay1/status", true);
#endif
#ifdef GSM_MINI_BOARD_V3
		digitalWrite(RELAY_OUT_A,HIGH);
#endif
	}
	else if (strncmp("Power?",smsbuffer,6)==0)
	{
		ret_value = 1;
		creat_power_sms(systemConfig.ac_power);
		//check access
		// 		if (sys_data.cli_access_level<2){ creatSMS("Unauthorized action",3); return; }
		// 		relay_B_deactivate();
	}
	else if (strncmp("chime1",smsbuffer,6)==0)
	{
		ret_value = 1;
		xEventGroupSetBits(EventRTOS_buzzer,    TASK_3_BIT );
		
	}
	else if (strncmp("siren=",smsbuffer,6)==0)
	{
		ret_value = 1;
		char zone_index[10];
		strlcpy(zone_index,smsbuffer+6,2);
		
		uint8_t state = atoi(zone_index);
	
		if(state){xEventGroupSetBits(EventRTOS_buzzer,    TASK_2_BIT );}
		else{
			xEventGroupSetBits(EventRTOS_buzzer,    TASK_1_BIT );
		}
		
		
	}
	return ret_value;
}