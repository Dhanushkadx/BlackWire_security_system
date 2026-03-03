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

bool setJson_key_bool(const char* path, const char* jkey, bool state) {
    DynamicJsonDocument doc(JSON_DOC_SIZE_CONFIG_DATA); 

    // Open file for reading
    File fileToRead = SPIFFS.open(path, FILE_READ);
    if (!fileToRead) {
        Serial.println(F("No file found, reset EEPROM"));
        return false; // Return false if file not found
    }

    // Deserialize JSON document
    DeserializationError error = deserializeJson(doc, fileToRead);
    fileToRead.close(); // Close the file after reading

    if (error) {
        Serial.print(F("Failed to read file, using default configuration"));
        return false; // Return false if deserialization fails
    }

    // Set the JSON key to the new state
    doc["sysconf"][jkey] = state;

    Serial.println(F("Saving config..."));

    // Open file for writing
    File fileToWrite = SPIFFS.open(path, FILE_WRITE);
    if (!fileToWrite) {
        Serial.println(F("Failed to open file for writing"));
        return false; // Return false if file could not be opened for writing
    }

    // Serialize JSON document to file
    if (serializeJson(doc, fileToWrite) == 0) {
        Serial.println(F("Failed to write to file"));
        fileToWrite.close();
        return false; // Return false if serialization fails
    }

    fileToWrite.close(); // Close the file after writing
    return true; // Return true if successful
}
	 
void configLoad(uint8_t mode)
{
    switch (mode)
    {

    // =====================================================
    // MODE 2 : Load zones and user database
    // =====================================================
    case 2:
    {
        Serial.println(F("Loading zone database..."));
        gZoneManager.begin(SPIFFS, "/zones.bin");
        gZoneManager.syncToEngine(zoneEngine);
        // -------------------------------------------------
        // Load remote database
        // ------------------------------------------------
            Serial.println(F("Loading remote database..."));
            if(RemoteStorage::begin(SPIFFS, "/remotes.bin")){
                Serial.println(F("RemoteStorage loaded successfully"));
            } else {
                Serial.println(F("RemoteStorage load failed"));
            }
#ifdef _DEBUG
          
            RemoteStorage::printToSerial();  
#endif
       

        // -------------------------------------------------
        // Load user database
        // -------------------------------------------------

        File usersFile = SPIFFS.open("/personx.json", "r");

        if (!usersFile) {
#ifdef _DEBUG
            Serial.println(F("Failed to open /personx.json"));
#endif
            break;
        }

        DynamicJsonDocument usersDoc(JSON_DOC_SIZE_USER_DATA);

        DeserializationError err = deserializeJson(usersDoc, usersFile);
        usersFile.close();

        if (err) {
#ifdef _DEBUG
            Serial.print(F("User JSON parse failed: "));
            Serial.println(err.c_str());
#endif
            break;
        }

        JsonObject usersObj = usersDoc["users"].as<JsonObject>();

        if (usersObj.isNull()) {
#ifdef _DEBUG
            Serial.println(F("Invalid format: missing users object"));
#endif
            break;
        }

#ifdef _DEBUG
        Serial.println(F("Loaded users:"));
        serializeJsonPretty(usersObj, Serial);
        Serial.println();
#endif

        break;
    }


    // =====================================================
    // MODE 0 : Full configuration load at boot
    // =====================================================
    case 0:
    {
        Serial.println(F("Loading system configuration..."));

        File fileToRead = SPIFFS.open("/config.json");

        if (!fileToRead) {
            Serial.println(F("config.json missing -> resetting config"));
            configReset();
            break;
        }

        DynamicJsonDocument doc(JSON_DOC_SIZE_CONFIG_DATA);

        DeserializationError err = deserializeJson(doc, fileToRead);
        fileToRead.close();

        if (err) {
            Serial.println(F("config.json parse error"));
            break;
        }

        JsonObject sys = doc["sysconf"];

        // -------------------------------------------------
        // System parameters
        // -------------------------------------------------

        systemConfig.battery_charging_en   = sys["battery_charging_en"];
        systemConfig.bell_time_out         = sys["bell_time_out"];
        systemConfig.beep_time_out         = sys["beep_time_out"];
        systemConfig.siren_en              = sys["siren_en"];
        systemConfig.beep_en               = sys["beep_en"];
        systemConfig.cli_access_level      = sys["cli_access_level"];
        systemConfig.entry_delay_time      = sys["entry_delay_time"];
        systemConfig.exit_delay_time       = sys["exit_delay_time"];
        systemConfig.sensor_debounce_time  = sys["debounce_time"];
        systemConfig.wifi_sta_en           = sys["wifi_sta_en"];
        systemConfig.mqtt_en               = sys["mqtt_en"];
        systemConfig.call_attempts         = sys["call_attempts"];
        systemConfig.call_en               = sys["call_en"];
        systemConfig.wifiap_en             = sys["wifiap_en"];

#ifdef GSM_PULSEX_IOT_BOARD
        bool reset_pin_state = true;
#else
        bool reset_pin_state = digitalRead(PROGRAM_PIN);
#endif

        // -------------------------------------------------
        // Determine system mode
        // -------------------------------------------------

        if (true)   // your forced CONFIG_MODE logic
        {
            strcpy(systemConfig.installer_pass, "admin");
            Serial.println(F("Installer password default"));
            system_mode = CONFIG_MODE;
        }
        else
        {
            if (systemConfig.wifi_sta_en)
            {
                const char* ssid = sys["wifissid_sta"];
                const char* pass = sys["wifipass"];

                strcpy(systemConfig.wifissid_sta, ssid);
                strcpy(systemConfig.wifipass, pass);

                const char* installerPW = sys["installer_pass"];
                strcpy(systemConfig.installer_pass, installerPW);

                system_mode = NOMAL_MODE_WIFI;

                Serial.println(F("WiFi credentials loaded"));
            }
            else
            {
                system_mode = NOMAL_MODE_NO_WIFI;
            }
        }

        // -------------------------------------------------
        // Restore last system state
        // -------------------------------------------------

        memset(systemConfig.last_sms_sender, '\0', 15);
        strcpy(systemConfig.last_sms_sender, sys["last_sms_sender"]);

        const char* lastState = sys["last_system_state"];

        if (strncmp("Home arm", lastState, 8) == 0) {
            systemConfig.last_system_state = SYS1_IDEAL;
        }
        else if (strncmp("Disarm", lastState, 6) == 0) {
            systemConfig.last_system_state = DEACTIVE;
        }

        systemConfig.last_system_state = SYS1_IDEAL;

#ifdef _DEBUG
        Serial.println(F("Loaded config.json"));
        serializeJsonPretty(doc, Serial);
        Serial.println();
#endif

        // -------------------------------------------------
        // Load zones
        // -------------------------------------------------

        Serial.println(F("Loading zone database..."));

        gZoneManager.begin(SPIFFS, "/zones.bin");
        gZoneManager.syncToEngine(zoneEngine);

         // -------------------------------------------------
        // Load remote database
        // ------------------------------------------------
            Serial.println(F("Loading remote database..."));
            if(RemoteStorage::begin(SPIFFS, "/remotes.bin")){
                Serial.println(F("RemoteStorage loaded successfully"));
                //RemoteStorage::learnFromCodeStr(0, "1234567890", true);
                //RemoteStorage::setEnabled(0, true);
            } else {
                Serial.println(F("RemoteStorage load failed"));
            }
            RemoteStorage::printToSerial();  
#ifdef _DEBUG
          
            RemoteStorage::printToSerial();  
#endif

        // -------------------------------------------------
        // Load users
        // -------------------------------------------------

        File usersFile = SPIFFS.open("/personx.json");

        if (usersFile) {
            DynamicJsonDocument usersDoc(JSON_DOC_SIZE_USER_DATA);
            deserializeJson(usersDoc, usersFile);

#ifdef _DEBUG
            serializeJsonPretty(usersDoc, Serial);
#endif

            usersFile.close();
        }

        // -------------------------------------------------
        // Reset temporary AP request flag
        // -------------------------------------------------

        if (systemConfig.wifiap_en)
        {
            Serial.println(F("Reverting temporary WiFi AP flag"));

            File cfg = SPIFFS.open("/config.json");
            if (!cfg) break;

            DynamicJsonDocument docx(JSON_DOC_SIZE_CONFIG_DATA);
            deserializeJson(docx, cfg);
            cfg.close();

            docx["sysconf"]["wifiap_en"] = false;

            File out = SPIFFS.open("/config.json", FILE_WRITE);
            serializeJson(docx, out);
            out.close();
        }

        break;
    }

    } // end switch
}
	
	 

	 

	 
void configReset(){
	 
}


void configSave(){
	DynamicJsonDocument doc(JSON_DOC_SIZE_CONFIG_DATA);
	
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
			sprintf(buff, "z0%d", index_start);
		}
		else {
			sprintf(buff, "z%d", index_start);
		}
		
//#ifdef _DEBUG

		Serial.printf("id:%d \n",index_start);
		
//#endif
		if (docx[buff]["ed"]) { any_sensor_array[index_start].device_state |= (1 << BIT_MASK_ENTRY_DELAY); }
		else { any_sensor_array[index_start].device_state &= ~(1 << BIT_MASK_ENTRY_DELAY); }

		if (docx[buff]["xd"]) { any_sensor_array[index_start].device_state |= (1 << BIT_MASK_EXIT_DELAY); }
		else { any_sensor_array[index_start].device_state &= ~(1 << BIT_MASK_EXIT_DELAY); }			 

		if (docx[buff]["by"]) {any_sensor_array[index_start].device_state |= (1 << BIT_MASK_BYPASSED); }
		else { any_sensor_array[index_start].device_state &= ~(1 << BIT_MASK_BYPASSED); }
		
		if (docx[buff]["rf"]) { any_sensor_array[index_start].device_type |= (1 << BIT_MASK_RF); }
		else { any_sensor_array[index_start].device_type &= ~(1 << BIT_MASK_RF); }
		
		if (docx[buff]["pm"]) { any_sensor_array[index_start].device_type |= (1 << BIT_MASK_PERIMETER); }
		else { any_sensor_array[index_start].device_type &= ~(1 << BIT_MASK_PERIMETER); }
		
		if (docx[buff]["x24"]) { any_sensor_array[index_start].device_type |= (1 << BIT_MASK_24H); }
		else { any_sensor_array[index_start].device_type &= ~(1 << BIT_MASK_24H); }
		
		if (docx[buff]["sl"]) { any_sensor_array[index_start].device_type |= (1 << BIT_MASK_SILENT); }
		else { any_sensor_array[index_start].device_type &= ~(1 << BIT_MASK_SILENT); }
		

	}
#ifdef _DEBUG
	 // This code will only be included in the Debug configuration
	 serializeJsonPretty(docx, Serial);
#endif
	
		fileToReadx.close();
}
