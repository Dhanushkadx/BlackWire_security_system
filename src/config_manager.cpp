/*
 * config_JSON.h
 *
 * Created: 11/6/2022 6:26:11 PM
 *  Author: dhanu
 */ 



#include "config_manager.h"


bool getJson_key_char(const char* path, const char* jkey, char*buffer, uint32_t size){
	 File fileToRead = SPIFFS.open(path);
	 if (!fileToRead)
	 {
		 Serial.println(F("no file found reset eeprom"));
		 return false;
	 }
	 
	 DynamicJsonDocument doc(2048);
	 deserializeJson(doc,  fileToRead);	 
	 const char* json_key = doc["sysconf"][jkey];
	 Serial.printf(PSTR("%s:%s\n"),jkey,json_key);
	 strcpy(buffer,json_key);
	 fileToRead.close();
	 return true;
}

bool getJson_key_int(const char* path, const char* jkey, uint32_t *number){
	 File fileToRead = SPIFFS.open(path);
	 if (!fileToRead)
	 {
		 Serial.println(F("no file found reset eeprom"));
		 return false;
	 }	 
	 DynamicJsonDocument doc(2048);
	 deserializeJson(doc,  fileToRead);	 
	 fileToRead.close();
	
        *number = doc["sysconf"][jkey];
		Serial.printf(PSTR("%s:%s\n"),jkey,number);
        return true;    
}

	 
 void configLoad(){	 
	
	 Serial.println(F("reload config data...................."));
	 File fileToRead = SPIFFS.open("/config.json");
	 if (!fileToRead)
	 {
		 Serial.println(F("no file found reset eeprom"));
		 configReset();
	 }
	 
	 DynamicJsonDocument doc(1024);
	 deserializeJson(doc,  fileToRead);	 
	 
	 systemConfig.battery_charging_en = doc["sysconf"]["battery_charging_en"];
	 systemConfig.bell_time_out = doc["sysconf"]["bell_time_out"];	
	 systemConfig.cli_access_level = doc["sysconf"]["cli_access_level"];
	 systemConfig.entry_delay_time = doc["sysconf"]["entry_delay_time"];
	 systemConfig.exit_delay_time = doc["sysconf"]["exit_delay_time"];
	 systemConfig.sensor_debounce_time = doc["sysconf"]["debounce_time"];
	 systemConfig.wifi_sta_en = doc["sysconf"]["wifi_sta_en"];
	 systemConfig.mqtt_en = doc["sysconf"]["mqtt_en"];
	/*char wifissid_ap[15]; //":"dxdxdxdxdx",
	 char wifissid_sta[15]; //":"dxdxdxdxdx",
	 char wifipass[15]; //": "xxxxxxxxxx",
	 uint8_t call_attempts;//": 2,
	 boolean call_en;  //": true,
	 char  installer_no[10];  //":"000000000000",
	 char installer_pass[10]   //":"000*/
	if (!digitalRead(PROGRAM_PIN))
	{		
		 strcpy(systemConfig.installer_pass, "admin");	
		 Serial.println(F("WiFi Password Default"));
		 system_mode = CONFIG_MODE;
	}
	else{

		if(systemConfig.wifi_sta_en==true){
		const char* ssid = doc["sysconf"]["wifissid_sta"];
		strcpy(systemConfig.wifissid_sta, ssid);	
		const char* pss = doc["sysconf"]["wifipass"];
		strcpy(systemConfig.wifipass, pss);	
		Serial.println(F("WiFi Password Set"));
		const char* installerPW = doc["sysconf"]["installer_pass"];
		strcpy(systemConfig.installer_pass, installerPW);
		system_mode = NOMAL_MODE_WIFI;
		}
		else{
			system_mode = NOMAL_MODE_NO_WIFI;
		}
	}
	 
	 memset(systemConfig.last_sms_sender,'\0',15);	 
	 strcpy(systemConfig.last_sms_sender, doc["sysconf"]["last_sms_sender"]);	
	 const char* last_sys_state = doc["sysconf"]["last_system_state"];
	 Serial.print("Last sys state");
	 Serial.println(last_sys_state);
	 
	 if (strncmp("Home arm",last_sys_state,8)==0)
	 {
		 systemConfig.last_system_state = SYS1_IDEAL;
	 }
	 else if (strncmp("Disarm",last_sys_state,6)==0)
	 {
		 systemConfig.last_system_state = DEACTIVE;
	 }
	 systemConfig.last_system_state = SYS1_IDEAL;
	 Serial.println(F("reload zone data...................."));
	 
#ifdef _DEBUG
	 // This code will only be included in the Debug configuration
	serializeJsonPretty(doc, Serial);
#endif	
	fileToRead.close();

	// load zone data 
	load_zones("/zone_data_8.json",0,8);
	load_zones("/zone_data_16.json",8,16);
	load_zones("/zone_data_24.json",16,24);
	load_zones("/zone_data_32.json",24,32);
	load_zones("/zone_data_40.json",32,40);
	load_zones("/zone_data_48.json",40,48);

	File users_fileToRead = SPIFFS.open("/personx.json");
	
	DynamicJsonDocument users_docx(JSON_DOC_SIZE_USER_DATA);
	deserializeJson(users_docx,  users_fileToRead);
#ifdef _DEBUG
	// This code will only be included in the Debug configuration
	serializeJsonPretty(users_docx, Serial);
#endif		
	users_fileToRead.close();		  
}
	 

	 
void configReset(){
	 
}


void configSave(){
	DynamicJsonDocument doc(1024);
	
	doc["battery_charging_en"] = systemConfig.battery_charging_en;
	doc["bell_time_out"] = systemConfig.bell_time_out;
	doc["cli_access_level"] = systemConfig.cli_access_level;
	doc["entry_delay_time"] = systemConfig.entry_delay_time;
	doc["exit_delay_time"] = systemConfig.exit_delay_time;
	doc["last_sms_sender"] = systemConfig.last_sms_sender;
	doc["last_system_state"] = systemConfig.last_system_state;
	doc["installer_pass"]= systemConfig.installer_pass;
	
	
	File fileToWrite = SPIFFS.open("/config.json", FILE_WRITE);
	
	serializeJson(doc,  fileToWrite);
	
	Serial.println("save config....................");
	serializeJsonPretty(doc, Serial);
	
	fileToWrite.close();
	
	
}


void load_zones(const char* path, uint8_t index_start, uint8_t index_end){

	Serial.printf_P(PSTR("**Read zones %d to %d path>%s\n"), index_start, index_end, path);

	File fileToReadx = SPIFFS.open(path);	
	DynamicJsonDocument docx(JSON_DOC_SIZE_ZONE_DATA);
	deserializeJson(docx,  fileToReadx);
	char buff[20] = {0};
	for (index_start; index_start<index_end; index_start++)
	{
		memset(buff, '\0', 20);
		if (index_start < 10) {
			sprintf(buff, "zone0%d", index_start);
		}
		else {
			sprintf(buff, "zone%d", index_start);
		}
		
//#ifdef _DEBUG

		const char *zone_name = docx[buff]["name"];
		Serial.printf("id:%d	name:%s \n",index_start,zone_name);
		
//#endif		
		//initially sensor creats in unavailable state, later we activate it based on hardware
        
		any_sensor_array[index_start].device_state &= ~(1 << BIT_MASK_AVAILABLE);

		if (docx[buff]["entry_delay"]) { any_sensor_array[index_start].device_state |= (1 << BIT_MASK_ENTRY_DELAY); }
		else { any_sensor_array[index_start].device_state &= ~(1 << BIT_MASK_ENTRY_DELAY); }

		if (docx[buff]["exit_delay"]) { any_sensor_array[index_start].device_state |= (1 << BIT_MASK_EXIT_DELAY); }
		else { any_sensor_array[index_start].device_state &= ~(1 << BIT_MASK_EXIT_DELAY); }			 

		if (docx[buff]["bypass"]) {any_sensor_array[index_start].device_state |= (1 << BIT_MASK_BYPASSED); }
		else { any_sensor_array[index_start].device_state &= ~(1 << BIT_MASK_BYPASSED); }
		
		if (docx[buff]["rf"]) { any_sensor_array[index_start].device_type |= (1 << BIT_MASK_RF); }
		else { any_sensor_array[index_start].device_type &= ~(1 << BIT_MASK_RF); }
		
		if (docx[buff]["perimeter"]) { any_sensor_array[index_start].device_type |= (1 << BIT_MASK_PERIMETER); }
		else { any_sensor_array[index_start].device_type &= ~(1 << BIT_MASK_PERIMETER); }
		
		if (docx[buff]["x24h"]) { any_sensor_array[index_start].device_type |= (1 << BIT_MASK_24H); }
		else { any_sensor_array[index_start].device_type &= ~(1 << BIT_MASK_24H); }
		
		if (docx[buff]["silent"]) { any_sensor_array[index_start].device_type |= (1 << BIT_MASK_SILENT); }
		else { any_sensor_array[index_start].device_type &= ~(1 << BIT_MASK_SILENT); }
		

	}
#ifdef _DEBUG
	 // This code will only be included in the Debug configuration
	 serializeJsonPretty(docx, Serial);
#endif
	
		fileToReadx.close();
}
