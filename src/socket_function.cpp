// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------
#include "socket_function.h"

AsyncWebSocket ws("/ws");

void notifyClients_pageInfo() {
	DynamicJsonDocument readings(512);
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
		readings["P1"] = "ACTIVE";
	}
	else
	{
		readings["P1"] = "DEACTIVE";
	}
	if(!client.connected()){
		readings["P9"] = "DISCONNECTED";
	}
	else{
		readings["P9"] = "CONNECTED";
	}

	
  	readings["P10"] = getSignal_strength();
 	readings["P2"] = rssi;
  	readings["P3"] = ipAddress;
	readings["P4"] = macAddress;
	// Loop through each sensor and dynamically generate keys "P0", "P1", "P2", ...
	for (int i = 0; i < 4; i++) {
	String key = "P" + String(i+5);  // Generate key like "P0", "P1", "P2", ...
	readings[key] = getSensor(i);    // Assign the state to the generated key
	}
	
  	String data;
	//data.reserve(12288);
	readings["respHeader"]= "pint";
	size_t len = serializeJson(readings, data);
	ws.textAll(data.c_str(),len);
	readings.garbageCollect();
	Serial.println("PG");
	
}

void notifyClients_pageZones() {
	File fileToReadx = SPIFFS.open("/zone_data_8.json");
	if (!fileToReadx)
	{
		Serial.println(F("couldn't open file"));
		return;
	}
	
	DynamicJsonDocument docx(JSON_DOC_SIZE_ZONE_DATA);
	DeserializationError err = deserializeJson(docx,  fileToReadx);
	fileToReadx.close();
	if (err) {
		Serial.print(F("deserializeJson() failed with code "));
		Serial.println(err.f_str());
		Serial.print(F("Free Memory after deserialization: "));
		Serial.println(ESP.getFreeHeap());
		// Get the task handle for the current task
		TaskHandle_t taskHandle = xTaskGetCurrentTaskHandle();
    
		// Print the task handle
		Serial.print("Current Task Handle: ");
		Serial.println((uintptr_t)taskHandle, DEC);  // Cast to uintptr_t for printing the task handle value
		return;
	}

	String data;
	//data.reserve(12288);
	docx["respHeader"]= "pint";
	size_t len = serializeJson(docx, data);
	//ws.textAll(data, len);
	
	/*char buffer[100];
	sprintf(buffer, "{\"zone01\":{\"name\":\"NILUS ROOM\"}}");*/
	Serial.print(F("send contact json and free heap>"));
	Serial.println(ESP.getFreeHeap());
	ws.textAll(data.c_str(),len);
	docx.garbageCollect();
	//Serial.println(data.c_str());
	
}

void notifyClients_pageUser() {
	File fileToReadx = SPIFFS.open("/personx.json");
	if (!fileToReadx)
	{
		Serial.println(F("couldn't open file"));
		return;
	}
	
	DynamicJsonDocument docx(JSON_DOC_SIZE_USER_DATA);
	DeserializationError err = deserializeJson(docx,  fileToReadx);
	fileToReadx.close();
	if (err) {
		Serial.print(F("deserializeJson() failed with code "));
		Serial.println(err.f_str());
		return;
	}

	String data;
	//data.reserve(12288);
	docx["respHeader"]= "pint";
	size_t len = serializeJson(docx, data);
	//ws.textAll(data, len);
	
	/*char buffer[100];
	sprintf(buffer, "{\"zone01\":{\"name\":\"NILUS ROOM\"}}");*/
	Serial.print(F("send contact json and free heap>"));
	Serial.println(ESP.getFreeHeap());
	ws.textAll(data.c_str(),len);
}

void notifyClients_pageSys() {
	File fileToReadx = SPIFFS.open("/config.json");
	if (!fileToReadx)
	{
		Serial.println(F("couldn't open file"));
		return;
	}
	
	DynamicJsonDocument docx(JSON_DOC_SIZE_CONFIG_DATA);
	DeserializationError err = deserializeJson(docx,  fileToReadx);
	fileToReadx.close();
	if (err) {
		Serial.print(F("deserializeJson() failed with code "));
		Serial.println(err.f_str());
		return;
	}

	String data;
	docx["respHeader"]= "pint";
	size_t len = serializeJson(docx, data);
	Serial.print(F("send config json and free heap>"));
	Serial.println(ESP.getFreeHeap());
	ws.textAll(data.c_str(),len);
	
}

void notifyClients_rfidRx(const char* data,uint32_t len){
	ws.textAll(data,len);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
	AwsFrameInfo *info = (AwsFrameInfo*)arg;
	if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
		
		//DynamicJsonDocument json_req(JSON_DOC_SIZE_DEVICE_DATA);
		// Dynamically allocate memory for JSON document based on incoming data length
        size_t json_doc_size = len + 100; // Adding 100 bytes as extra padding for deserialization overhead
        DynamicJsonDocument json_req(json_doc_size);

		DeserializationError err = deserializeJson(json_req, data);
		if (err) {
			Serial.print(F("deserializeJson() failed with code "));
			Serial.println(err.c_str());
			return;
		}
		
		const char *cmd = json_req["command"];
		Serial.println(F("action is command"));
		DynamicJsonDocument json_resp(150);
		

		const char *action = json_req["action"];
		if (strcmp(action, "submit") == 0) {// for now index page uses ws to send submit
			Serial.println(F("action is submit"));
			notifyClients_pageZones();
		}
		if (strcmp(action, "pint") == 0) {
			Serial.println(F("page initiate"));
			notifyClients_pageZones();
		}
		if (strcmp(action, "users") == 0) {
			Serial.println(F("users"));
			notifyClients_pageUser();
		}
		if (strcmp(action, "sys") == 0) {
			Serial.println(F("sys"));
			notifyClients_pageSys();
		}
		if (strcmp(action, "info") == 0) {
			Serial.println(F("info"));
			notifyClients_pageInfo();
		}
			
		
		if (strcmp(action, "cmd") == 0) {		
			
			if (strcmp(cmd , "bt0")==0)
			{
				uint8_t device_index = json_req["data0"];
				json_resp["respHeader"] = "data";
				json_resp["scan_rfid"] = get_device_RFID(device_index);
			}
			else if (strcmp(cmd , "bt1")==0)
			{
				json_resp["respHeader"] = "cmdok";
				Serial.println(F("action is command creat rf task"));
				xTaskCreate(Task6code,"Task6",5000,NULL,6,&Task6);
			}
			else if (strcmp(cmd , "bt2")==0)
			{
				json_resp["respHeader"] = "cmdok";
				const char *rfid = json_req["data0"];
				uint8_t zone_id = json_req["data1"];
				Serial.print(F("Zone> "));
				Serial.print(zone_id);
				Serial.print(F(" Rx Rf ID>"));
				Serial.println(rfid);
				char rfid_buff[15];
				strcpy(rfid_buff,rfid);
				set_device_RFID(zone_id, rfid_buff);
			}

		}
		if (strcmp(action, "pg_system_cmd") == 0) {
			const char *cmd = json_req["command"];
			Serial.println(F("action is command"));
			if (strcmp(cmd , "bt0")==0)
			{
				digitalWrite(RELAY_ALARM , !digitalRead(RELAY_ALARM));
			}
			if (strcmp(cmd , "bt1")==0)
			{
				myAlarm_pannel.set_arm_mode(AS_ITIS_NO_BYPASS);
				myAlarm_pannel.set_system_state(SYS1_IDEAL,WEB,0);
				publish_system_state("ARMED","info/mode",true);
			}
			if (strcmp(cmd , "bt2")==0)
			{
				eCurrent_state=DEACTIVE;
				myAlarm_pannel.set_system_state(DEACTIVE,WEB,0);
				 publish_system_state("DISARMED","info/mode",true);
			}
		}
		if (strcmp(action, "pg_contacts_cmd") == 0) {
			const char *cmd = json_req["command"];
			Serial.println(F("action is command"));
			if (strcmp(cmd , "p2bt0")==0)
			{
				Serial.print(F("p2bt0"));
				uint8_t device_index = json_req["data0"];
				json_resp["respHeader"] = "data";
				json_resp["scan_rfid"] = get_remote_RFID(device_index);
			}
			else if (strcmp(cmd , "p2bt1")==0)
			{
				json_resp["respHeader"] = "cmdok";
				Serial.println(F("action is command creat rf task"));
				xTaskCreate(Task6code,"Task6",4000,NULL,6,&Task6);
			}
			else if (strcmp(cmd , "p2bt2")==0)
			{
				json_resp["respHeader"] = "cmdok";
				const char *rfid = json_req["data0"];
				uint8_t zone_id = json_req["data1"];
				Serial.print(F("Remote> "));
				Serial.print(zone_id);
				Serial.print(F(" Rx Rem ID>"));
				Serial.println(rfid);
				char rfid_buff[15];
				strcpy(rfid_buff,rfid);
				set_remote_RFID(zone_id, rfid_buff);
			}
			
		}		
		String data;
		size_t len = serializeJson(json_resp, data);
		ws.textAll(data.c_str(), len);
	}
}

void onEvent(AsyncWebSocket       *server,
AsyncWebSocketClient *client,
AwsEventType          type,
void                 *arg,
uint8_t              *data,
size_t                len) {

	switch (type) {
		case WS_EVT_CONNECT:
		Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
		
		break;
		case WS_EVT_DISCONNECT:
		Serial.printf("WebSocket client #%u disconnected\n", client->id());
		break;
		case WS_EVT_DATA:
		handleWebSocketMessage(arg, data, len);
		break;
		case WS_EVT_PONG:
		case WS_EVT_ERROR:
		break;
	}
}


