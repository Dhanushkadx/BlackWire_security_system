#include "universalEventx.h"
#include "universalEvent_commands.h"

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
	
	if (strncmp(smsbuffer, UniversalCmd::kSysLog, sizeof(UniversalCmd::kSysLog) - 1)==0)
	{
		Serial.println(F("Reading Log"));
        delay(500);
		//processOfflineMessagesV2();
		readLastNMessagesToQueue(10);
		processMessagesFromQueue();
		Serial.println("loop2 exit");
		
	}
	if (strncmp(smsbuffer, UniversalCmd::kSysReboot, sizeof(UniversalCmd::kSysReboot) - 1)==0)
	{
		Serial.println(F("Restart in 5 sec"));
        delay(5000);
		ESP.restart();
	}
    if (strncmp(smsbuffer, UniversalCmd::kNetApEnable, sizeof(UniversalCmd::kNetApEnable) - 1)==0)
	{
        if(setJson_key_bool("/config.json", "wifiap_en", true)){
            Serial.println(F("AP setup ok"));
        }
		Serial.println(F("Restart in 5 sec"));        
        delay(5000);
		ESP.restart();
	}
	
	if (strncmp(smsbuffer, UniversalCmd::kNetQuery, sizeof(UniversalCmd::kNetQuery) - 1)==0)
	{
		
		Serial.print(F("Network>"));
		Serial.println(netowrk_operator_name_char);
	}
	if (strncmp(smsbuffer, UniversalCmd::kInfoPrefix, sizeof(UniversalCmd::kInfoPrefix) - 1)==0)
	{
		
		
	}
	if (strncmp(smsbuffer, UniversalCmd::kSysBlock, sizeof(UniversalCmd::kSysBlock) - 1)==0)
	{
		while(1){}
		
	}
	
	
	if (strncmp(smsbuffer, UniversalCmd::kAuthPrefix, sizeof(UniversalCmd::kAuthPrefix) - 1)==0)
	{
		ret_value = 1;
		char psw_char[6];
		strlcpy(psw_char,smsbuffer + (sizeof(UniversalCmd::kAuthPrefix) - 1),5);
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
	if (strncmp(smsbuffer, UniversalCmd::kZonePrefix, sizeof(UniversalCmd::kZonePrefix) - 1)==0 &&
		strstr(smsbuffer, UniversalCmd::kZoneNameTag) != nullptr)
	{
		ret_value = 1;
		char zone_cmd[40] = {0};
		char zone_index[5] = {0};
		char zone_name[16] = {0};
		strlcpy(zone_cmd, smsbuffer, sizeof(zone_cmd));
		char* token = strtok(zone_cmd, "=,");
		token = strtok(nullptr, "=,");
		if (token == nullptr) return ret_value;
		strlcpy(zone_index, token, sizeof(zone_index));
		token = strtok(nullptr, "=,");
		if (token == nullptr) return ret_value;
		token = strtok(nullptr, "");
		if (token == nullptr) return ret_value;
		strlcpy(zone_name, token, sizeof(zone_name));
		uint8_t zone_index_int = atoi(zone_index);
		set_device_name(zone_index_int, zone_name);
		Serial.printf_P(PSTR("**zone renaming**\n"));
		if (Invoker==GSM_MODULE)
		{
			char buffer[5];
			strcpy_P(buffer,PSTR("OK"));
			creatSMS(buffer,2,0);
		}
	}	
	
	if (strncmp(smsbuffer, UniversalCmd::kZonePrefix, sizeof(UniversalCmd::kZonePrefix) - 1)==0 &&
		strstr(smsbuffer, UniversalCmd::kZoneNameTag) == nullptr)
	{
		ret_value = 1;
		 set_zone_param(smsbuffer);
		 eeprom_load(0);
		 char reply_buff[5];
		 strcpy_P(reply_buff,PSTR("OK"));
		 creatSMS(reply_buff,2,0);
	}
	if (strncmp(smsbuffer, UniversalCmd::kSysStatus, sizeof(UniversalCmd::kSysStatus) - 1)==0)
	{
		ret_value = 1;
		char msg[200] = {0};
		String ipAddress = WiFi.localIP().toString();
  		String macAddress = WiFi.macAddress();
  		int32_t rssi = WiFi.RSSI();
		int8_t gsmrssi =  getSignal_strength();
		String wifiStatus;
		(!WiFi.isConnected())?wifiStatus = "offline": wifiStatus = "online";
  		sprintf(msg, "WiFi:%s\nSSID:%s\nPW:%s\nIP:%s\nMAC:%s\nWiFi RSSI:%d dBm\nGSM SIG:%d\n",wifiStatus,systemConfig.wifissid_sta,systemConfig.wifipass, ipAddress.c_str(), macAddress.c_str(), rssi,gsmrssi);
		//get time from esp32
			struct tm timeinfo = rtc.getTimeStruct();

			const char* daysOfWeek[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
			const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
									"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

			char buffer[40];
			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer), "%02d(%s)-%s-%04d %02d:%02d:%02d",
					timeinfo.tm_mday,
					daysOfWeek[timeinfo.tm_wday],
					months[timeinfo.tm_mon],
					timeinfo.tm_year + 1900,
					timeinfo.tm_hour,
					timeinfo.tm_min,
					timeinfo.tm_sec);

			Serial.println(buffer);
		
		strcat(msg,"Time: ");
		strcat(msg, buffer);
		strcat(msg,"\n");
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
	if (strncmp(smsbuffer, UniversalCmd::kArmHome, sizeof(UniversalCmd::kArmHome) - 1)==0)
	{
		ret_value = 1;
		myAlarm_pannel.set_arm_mode(AS_ITIS_NO_BYPASS);
		myAlarm_pannel.set_system_state(SYS1_IDEAL,Invoker,user_id);
#ifdef MQTT_OK
		publish_system_state("ARMED","info/mode",true);
#endif       
		
	}
	else if(strncmp(smsbuffer, UniversalCmd::kArmDisarm, sizeof(UniversalCmd::kArmDisarm) - 1)==0){
		ret_value = 1;	
		eCurrent_state=DEACTIVE;
		myAlarm_pannel.set_system_state(DEACTIVE,Invoker,user_id);
#ifdef MQTT_OK
		publish_system_state("DISARMED","info/mode",true);
#endif        
		
	}
	
	else if(strncmp(smsbuffer, UniversalCmd::kArmPanic, sizeof(UniversalCmd::kArmPanic) - 1)==0)
	{
		ret_value =1;
		myAlarm_pannel.set_system_state(PANIC,Invoker,user_id);
	}

	else if(strncmp(smsbuffer, UniversalCmd::kArmAlarm, sizeof(UniversalCmd::kArmAlarm) - 1)==0)
	{
		ret_value =1;
		//myAlarm_pannel.set_system_state(PANIC,Invoker,user_id);
		myAlarm_pannel.set_system_state(ALARM_CALLING,WEB,0);
#ifdef MQTT_OK
		publish_system_state("ALARM","info/mode",true);
#endif   
	}
	
	else if(strncmp(smsbuffer, UniversalCmd::kPhoneSet, sizeof(UniversalCmd::kPhoneSet) - 1)==0)
	{
		ret_value = 1;
		//check access
		//if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3); }
		
		 char number[25];
		 memset(number,'\0',25);
		 strcpy(number, smsbuffer + (sizeof(UniversalCmd::kPhoneSet) - 1));
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
	else if(strncmp(smsbuffer, UniversalCmd::kPhoneSms, sizeof(UniversalCmd::kPhoneSms) - 1)==0){
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
	else if(strncmp(smsbuffer, UniversalCmd::kPhoneCall, sizeof(UniversalCmd::kPhoneCall) - 1)==0){
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
	
	
	else if(strncmp(smsbuffer, UniversalCmd::kRfLearnB, sizeof(UniversalCmd::kRfLearnB) - 1)==0){
		
		
	}
	else if (strncmp(smsbuffer, UniversalCmd::kRfId, sizeof(UniversalCmd::kRfId) - 1) == 0) {
	ret_value = 1;
	//RFbaster(smsbuffer);
	//send_rfid_state_update_to_mqtt(smsbuffer);
	}
	
	else if(strncmp(smsbuffer, UniversalCmd::kSysReset, sizeof(UniversalCmd::kSysReset) - 1)==0){
		ret_value = 1;
		//check access
		//if (sys_data.cli_access_level<2){ creatSMS("Unauthorized action",3); return; }
		Serial.println(F("EEPROM Reset"));
		configReset(); configLoad(0);
		
	}
	
	else if (strncmp(smsbuffer, UniversalCmd::kInfoPrefix, sizeof(UniversalCmd::kInfoPrefix) - 1)==0)
	{
		
		
	}	
	else if (strncmp(smsbuffer, UniversalCmd::kCfgEntryDelay, sizeof(UniversalCmd::kCfgEntryDelay) - 1)==0)
	{
		ret_value = 1;
		//check access
		if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3,0); }
		
		char zone_index[10];
		
		strlcpy(zone_index, smsbuffer + (sizeof(UniversalCmd::kCfgEntryDelay) - 1), 3);
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
	else if (strncmp(smsbuffer, UniversalCmd::kCfgExitDelay, sizeof(UniversalCmd::kCfgExitDelay) - 1)==0)
	{
		ret_value = 1;
		//check access
		if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3,0); }
		
		char zone_index[10];
		strlcpy(zone_index, smsbuffer + (sizeof(UniversalCmd::kCfgExitDelay) - 1), 3);
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
	
	else if (strncmp(smsbuffer, UniversalCmd::kOutputPrefix, sizeof(UniversalCmd::kOutputPrefix) - 1)==0)
	{
		ret_value = 1;
		char out_buffer[20] = {0};
		strlcpy(out_buffer, smsbuffer + (sizeof(UniversalCmd::kOutputPrefix) - 1), sizeof(out_buffer));
		char* out_index_str = strtok(out_buffer, ",");
		char* out_state_str = strtok(nullptr, ",");
		if ((out_index_str == nullptr) || (out_state_str == nullptr)) {
			return 0;
		}

		const int out_index = atoi(out_index_str);
		const bool turn_on = (out_state_str[0] == '1');
		char buffer[25];
		memset(buffer, 0, sizeof(buffer));

		if (out_index == 2) {
			strcpy_P(buffer, turn_on ? PSTR("Relay 2 on OK") : PSTR("Relay 2 off OK"));
			creatSMS(buffer,2,0);
#ifdef MQTT_OK
			publish_system_state(turn_on ? "on" : "off","cmd/relay2/status", true);
#endif
#ifdef GSM_MINI_BOARD_V3
			digitalWrite(RELAY_OUT_B, turn_on ? LOW : HIGH);
#endif
		} else if (out_index == 1) {
			strcpy_P(buffer, turn_on ? PSTR("Relay 1 on OK") : PSTR("Relay 1 off OK"));
			creatSMS(buffer,2,0);
#ifdef MQTT_OK
			publish_system_state(turn_on ? "on" : "off","cmd/relay1/status", true);
#endif
#ifdef GSM_MINI_BOARD_V3
			digitalWrite(RELAY_OUT_A, turn_on ? LOW : HIGH);
#endif
		} else {
			ret_value = 0;
		}
	}
	else if (strncmp(smsbuffer, UniversalCmd::kPowerQuery, sizeof(UniversalCmd::kPowerQuery) - 1)==0)
	{
		ret_value = 1;
		creat_power_sms(systemConfig.ac_power);
		//check access
		// 		if (sys_data.cli_access_level<2){ creatSMS("Unauthorized action",3); return; }
		// 		relay_B_deactivate();
	}
	else if (strncmp(smsbuffer, UniversalCmd::kBuzzChime, sizeof(UniversalCmd::kBuzzChime) - 1)==0)
	{
		ret_value = 1;
		xEventGroupSetBits(EventRTOS_buzzer,    TASK_3_BIT );
		
	}
	else if (strncmp(smsbuffer, UniversalCmd::kSirenPrefix, sizeof(UniversalCmd::kSirenPrefix) - 1)==0)
	{
		ret_value = 1;
		char zone_index[10];
		strlcpy(zone_index, smsbuffer + (sizeof(UniversalCmd::kSirenPrefix) - 1), 2);
		
		uint8_t state = atoi(zone_index);
	
		if(state){xEventGroupSetBits(EventRTOS_buzzer,    TASK_2_BIT );}
		else{
			xEventGroupSetBits(EventRTOS_buzzer,    TASK_1_BIT );
		}
		
		
	}
	return ret_value;
}
