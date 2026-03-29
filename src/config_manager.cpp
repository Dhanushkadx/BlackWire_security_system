/*
 * config_JSON.h
 *
 * Created: 11/6/2022 6:26:11 PM
 *  Author: dhanu
 */ 



#include "config_manager.h"
#include "mqtt_brokerx.h"

static JsonObject get_config_root(JsonDocument& doc) {
  return doc.as<JsonObject>();
}


bool getJson_key_char(const char* path, const char* jkey, char* buffer, uint32_t size) {
  File fileToRead = SPIFFS.open(path);
  if (!fileToRead) {
    Serial.printf("getJson_key_char: file not found: %s\n", path);
    return false;
  }
  DynamicJsonDocument doc(2048);
  deserializeJson(doc, fileToRead);
  fileToRead.close();
  // Use safe fallback — key may be absent from config
  JsonObject cfg = get_config_root(doc);
  const char* value = cfg[jkey] | "";
  strlcpy(buffer, value, size);
  Serial.printf("config read  %s = \"%s\"\n", jkey, buffer);
  return true;
}

bool getJson_key_int(const char* path, const char* jkey, uint32_t* number) {
  File fileToRead = SPIFFS.open(path);
  if (!fileToRead) {
    Serial.printf("getJson_key_int: file not found: %s\n", path);
    return false;
  }
  DynamicJsonDocument doc(2048);
  deserializeJson(doc, fileToRead);
  fileToRead.close();
  JsonObject cfg = get_config_root(doc);
  *number = cfg[jkey] | (uint32_t)0;
  Serial.printf("config read  %s = %lu\n", jkey, (unsigned long)*number);
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
    doc[jkey] = state;

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

        File usersFile = SPIFFS.open("/users.json", "r");

        if (!usersFile) {
#ifdef _DEBUG
            Serial.println(F("Failed to open /users.json"));
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

#ifdef _DEBUG
        Serial.println(F("Loaded users:"));
        serializeJsonPretty(usersDoc, Serial);
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
            Serial.printf("config.json parse error: %s\n", err.c_str());
            break;
        }

        Serial.println(F("config.json raw content:"));
        serializeJsonPretty(doc, Serial);
        Serial.println();

        JsonObject cfg = get_config_root(doc);
        if (cfg.isNull()) {
            Serial.println(F("config.json missing config/sysconf object"));
            break;
        }

        // -------------------------------------------------
        // Version and timestamp
        // -------------------------------------------------
        systemConfig.config_ver        = cfg["ver"]       | cfg["version"]   | (uint32_t)0;
        systemConfig.config_updated_ts = cfg["updatedTs"] | cfg["updatedAt"] | (uint64_t)0;

        // -------------------------------------------------
        // System parameters
        // -------------------------------------------------
        systemConfig.entry_delay_time     = cfg["enDelay"]        | (uint8_t)0;
        systemConfig.exit_delay_time      = cfg["xtDelay"]        | (uint8_t)0;
        systemConfig.bell_time_out        = cfg["bellTout"]       | (uint16_t)0;
        systemConfig.beep_time_out        = cfg["beepTout"]       | (uint16_t)0;
        systemConfig.sensor_debounce_time = cfg["debTm"]          | (uint8_t)0;
        systemConfig.siren_en             = cfg["bellEn"]         | cfg["siren_en"]      | false;
        systemConfig.beep_en              = cfg["beepEn"]         | cfg["beep_en"]       | false;
        systemConfig.call_en              = cfg["callEn"]         | cfg["call_en"]       | false;
        systemConfig.call_attempts        = cfg["callAtmpt"]      | cfg["call_attempts"] | (uint8_t)0;
        systemConfig.mqtt_en              = cfg["mqttEn"]      | cfg["mqtt_en"]           | false;
        systemConfig.cli_access_level     = cfg["cliLevel"]    | cfg["cli_access_level"]  | (uint8_t)0;
        systemConfig.wifi_sta_en          = cfg["wstaEn"]      | cfg["wifi_sta_en"]        | false;
        systemConfig.wifiap_en            = cfg["wapEn"]       | cfg["wifiap_en"]          | false;
        strlcpy(systemConfig.inst_no,  cfg["instNo"]  | "", sizeof(systemConfig.inst_no));
        strlcpy(systemConfig.inst_pas, cfg["instPas"] | "", sizeof(systemConfig.inst_pas));
        strlcpy(systemConfig.wifissid_ap,     cfg["wapssid"]        | "", sizeof(systemConfig.wifissid_ap));
        strlcpy(systemConfig.wifipass_ap,     cfg["wapPw"]          | "", sizeof(systemConfig.wifipass_ap));
        strlcpy(systemConfig.last_sms_sender, cfg["lastSender"] | cfg["last_sms_sender"] | "", sizeof(systemConfig.last_sms_sender));
        // Local MQTT credentials — only used when MQTT_SECURE is NOT defined
        strlcpy(systemConfig.mqtt_server, cfg["mqttServer"] | "", sizeof(systemConfig.mqtt_server));
        systemConfig.mqtt_port = cfg["mqttPort"] | (uint16_t)1883;
        strlcpy(systemConfig.mqtt_user,   cfg["mqttUser"]   | "", sizeof(systemConfig.mqtt_user));
        strlcpy(systemConfig.mqtt_pass,   cfg["mqttPass"]   | "", sizeof(systemConfig.mqtt_pass));
        // Entry / exit delay feature flags
        systemConfig.et_en   = cfg["etEn"]   | true;
        systemConfig.et_beep = cfg["etBeep"] | true;
        systemConfig.xt_en   = cfg["xtEn"]   | true;
        systemConfig.xt_beep = cfg["xtBeep"] | true;
        // Boot arm state
        strlcpy(systemConfig.sys_mode, cfg["sysMode"] | "disarm", sizeof(systemConfig.sys_mode));
#ifdef CUSTOM_NETWORK_CONFIG
        strlcpy(systemConfig.wbssid, cfg["wbssid"] | "", sizeof(systemConfig.wbssid));
#endif

#ifdef GSM_PULSEX_IOT_BOARD
        bool reset_pin_state = true;
#else
        bool reset_pin_state = digitalRead(PROGRAM_PIN);
#endif

        // -------------------------------------------------
        // Determine system mode
        // -------------------------------------------------

        // -------------------------------------------------
        // Determine system mode and load WiFi credentials
        // -------------------------------------------------
        // Always load WiFi credentials — needed for configSave() to not overwrite with empty
        strlcpy(systemConfig.wifissid_sta, cfg["wssid"]  | cfg["wifissid_sta"] | "", sizeof(systemConfig.wifissid_sta));
        strlcpy(systemConfig.wifipass,     cfg["wstaPw"] | cfg["wifipass"]     | "", sizeof(systemConfig.wifipass));

#ifdef FORCE_SYS_MODE
        system_mode = (eSYS_MODE)(FORCE_SYS_MODE);
        Serial.println(F("[FORCE_SYS_MODE] compile-time mode override active"));
#else
        if (systemConfig.wifiap_en){
            system_mode = CONFIG_MODE;
            Serial.println(F("AP mode enabled -> CONFIG_MODE"));
        } else if (systemConfig.wifi_sta_en) {
            system_mode = NOMAL_MODE_WIFI;
            Serial.println(F("WiFi credentials loaded -> NOMAL_MODE_WIFI"));
        } else {
            system_mode = NOMAL_MODE_NO_WIFI;
            Serial.println(F("WiFi disabled -> NOMAL_MODE_NO_WIFI"));
        }
#endif

#ifdef _DEBUG
        Serial.println(F("Loaded config.json"));
        serializeJsonPretty(doc, Serial);
        Serial.println();
#endif

        // -------------------------------------------------
        // Load zones
        // -------------------------------------------------

        Serial.println(F("Loading cfgIndex..."));
        mqtt_load_cfg_index();

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

        File usersFile = SPIFFS.open("/users.json");

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

            docx["wapEn"] = false;

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

	doc["ver"]       = systemConfig.config_ver;
	doc["updatedTs"] = systemConfig.config_updated_ts;
	doc["enDelay"]   = systemConfig.entry_delay_time;
	doc["xtDelay"]   = systemConfig.exit_delay_time;
	doc["bellTout"]  = systemConfig.bell_time_out;
	doc["beepTout"]  = systemConfig.beep_time_out;
	doc["debTm"]     = systemConfig.sensor_debounce_time;
	doc["bellEn"]    = systemConfig.siren_en;
	doc["beepEn"]    = systemConfig.beep_en;
	doc["callEn"]    = systemConfig.call_en;
	doc["callAtmpt"] = systemConfig.call_attempts;
	doc["cliLevel"]  = systemConfig.cli_access_level;
	doc["lastSender"]      = systemConfig.last_sms_sender;
	doc["instNo"]  = systemConfig.inst_no;
	doc["instPas"] = systemConfig.inst_pas;
	doc["wstaEn"]    = systemConfig.wifi_sta_en;
	doc["wapEn"]     = systemConfig.wifiap_en;
	doc["wssid"]     = systemConfig.wifissid_sta;
	doc["wapssid"]   = systemConfig.wifissid_ap;
	doc["wstaPw"]    = systemConfig.wifipass;
	doc["wapPw"]     = systemConfig.wifipass_ap;
	doc["mqttEn"]     = systemConfig.mqtt_en;
	doc["mqttServer"] = systemConfig.mqtt_server;
	doc["mqttPort"]   = systemConfig.mqtt_port;
	doc["mqttUser"]   = systemConfig.mqtt_user;
	doc["mqttPass"]   = systemConfig.mqtt_pass;
	doc["etEn"]       = systemConfig.et_en;
	doc["etBeep"]     = systemConfig.et_beep;
	doc["xtEn"]       = systemConfig.xt_en;
	doc["xtBeep"]     = systemConfig.xt_beep;
	doc["sysMode"]    = systemConfig.sys_mode;
#ifdef CUSTOM_NETWORK_CONFIG
	doc["wbssid"]     = systemConfig.wbssid;
#endif

	File fileToWrite = SPIFFS.open("/config.json", FILE_WRITE);
	serializeJson(doc, fileToWrite);
	fileToWrite.close();

	Serial.println(F("configSave done"));
	serializeJsonPretty(doc, Serial);
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
