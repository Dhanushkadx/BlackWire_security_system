
#include "mqtt_broker.h"

//sensor status 
uint8_t zone; 
bool state;

#ifdef MQTT_SECURE
// load DigiCert Global Root CA ca_cert
const char * ca_cert = \
  "-----BEGIN CERTIFICATE-----\n"\
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n"\
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"\
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n"\
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n"\
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"\
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n"\
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n"\
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n"\
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n"\
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n"\
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n"\
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n"\
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n"\
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n"\
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n"\
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n"\
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n"\
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n"\
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n"\
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4="\
"-----END CERTIFICATE-----\n";

// init secure wifi client
WiFiClientSecure espClient;
// MQTT Broker

const char* mqttServer = "j0117d13.ala.asia-southeast1.emqxsl.com";
const int mqtt_port = 8883;
const char *mqtt_username = "gsmesp32";
const char *mqtt_password = "12345";

// creat dead message
uint8_t mac[6];
char device_id_macStr[18];



#else
// init secure wifi client
WiFiClient espClient;
// MQTT Broker
char mqttServer[100] = {0};
char mqtt_username[100] = {0};
char mqtt_password[100] = {0};
uint32_t mqtt_port;
//const char* mqttServer = "192.168.1.200";
//onst int mqtt_port = 1883;
//const char *mqtt_username = "dhanushkadx";
//const char *mqtt_password = "cyclone10153";
#endif
bool mqtt_enable=false;
// use wifi client to init mqtt client
PubSubClient client(espClient);

_callbackFunctionType7 fn_onMQTT_connection;
_callbackFunctionType7 fn_onMQTT_disconnection;

void setup_mqtt(){
  
  
  if(!mqtt_enable){Serial.println(F("MQTT DISABLED")); return;}
  else{
    Serial.printf_P(PSTR("MQTT_SERVR:%s\nPORT:%d\nUSER:%s\nPASS:%s"),mqttServer,mqtt_port,mqtt_username,mqtt_password);
  }
  //creta id
  WiFi.macAddress(mac);  
  // Format the MAC address without colons and with underscores
  sprintf(device_id_macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      // set root ca cert
#ifdef MQTT_SECURE
  espClient.setCACert(ca_cert);
#else
// get mqtt credintial from spif
  if(!getJson_key_char("/config.json", "mqtt_server", mqttServer, 100)){
    Serial.println(F("MQTT CONFIG LOAD ERROR"));
  }
  if(!getJson_key_char("/config.json", "mqtt_user", mqtt_username, 100)){
    Serial.println(F("MQTT CONFIG LOAD ERROR"));
  }
  if(!getJson_key_char("/config.json", "mqtt_pass", mqtt_password, 100)){
    Serial.println(F("MQTT CONFIG LOAD ERROR"));
  }
  if(!getJson_key_int("/config.json", "mqtt_port", &mqtt_port)){
    Serial.println(F("MQTT CONFIG LOAD ERROR"));
  }
#endif
  client.setKeepAlive(60);
  // connecting to a mqtt broker
  client.setServer(mqttServer, mqtt_port);
  client.setCallback(callback);   
}


void callback_onMQTT_connection(_callbackFunctionType7 pFn){fn_onMQTT_connection = pFn;}
void callback_onMQTT_disconnection(_callbackFunctionType7 pFn){fn_onMQTT_disconnection = pFn;}


void reconnectMQTT() {
  if(!mqtt_enable){Serial.println(F("MQTT DISABLED")); return;}
  int state =WiFi.status();
  if(state !=WL_CONNECTED)
  {
      Serial.println(F("No WiFi to Reconnect MQTT"));
      if(fn_onMQTT_disconnection!=nullptr){fn_onMQTT_disconnection();}
      return;
  }

  String client_id = "blackwire-";
  client_id += String(device_id_macStr);
  Serial.printf_P(PSTR("MQTT client id: %s\n"), client_id.c_str());

  char lastwill_topic[50];
  memset(lastwill_topic,'\0',50);
  sprintf_P(lastwill_topic,PSTR("blackwire/%s/info/status"),device_id_macStr);

  while (!client.connected()) {
    vTaskDelay(5000/portTICK_PERIOD_MS);
    Serial.printf("Reconnecting to MQTT broker... at %s\n",mqttServer);
     if(fn_onMQTT_disconnection!=nullptr){fn_onMQTT_disconnection();}
     // set root ca cert
#ifdef MQTT_SECURE
  espClient.setCACert(ca_cert);
#endif
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password,lastwill_topic, 1, true, "offline")) {
        Serial.println(F("Connected to MQTT broker."));
        client.publish(lastwill_topic,"online",true);
        publish_system_state(WiFi.localIP().toString().c_str(),"info/ip",true);
        setup_subscriptions();
        if(fn_onMQTT_connection!=nullptr){fn_onMQTT_connection();}

    } else {
        Serial.print(F("Failed to reconnect to MQTT broker, rc="));
        Serial.print(client.state());
        Serial.println(F("Retrying in 5 seconds."));
        delay(5000);
    }
    int state =WiFi.status();
    if(state !=WL_CONNECTED)
        {
            Serial.println(F("No WiFi to Reconnect MQTT"));
            return;
        }
  }
}


	
void send_sensor_state_update_to_mqtt(uint8_t _zone,bool _state){
       
    char topic[50];
    memset(topic,'\0',50);
    sprintf_P(topic,PSTR("sensors/z%d"),_zone);
    if(_state){ publish_system_state("true", topic, true);}
    else{ publish_system_state("false", topic, true);}
   

}



void send_rfid_state_update_to_mqtt(const char* rfid){
       
   publish_system_state(rfid, "info/rfid", false);

}


void publish_system_state(const char* state, const char* subtopic, bool retaind_flag){

    char topic[50];
    memset(topic,'\0',50);
    sprintf_P(topic,PSTR("blackwire/%s/%s"),device_id_macStr,subtopic);
    if(!mqtt_enable){
      #ifdef _DEBUG
            Serial.println(F("MQTT DISABLED")); 
      #endif
            return;
          }
      #ifdef _DEBUG
          Serial.printf_P(PSTR("MQTT - Update - topic>%s \n"), topic);
      #endif
        client.publish(topic, state,retaind_flag);
}

void publish_incomming_sms_to_mqtt(char* local_smsbuffer, char* n ){
  DynamicJsonDocument doc(200);
		doc["msg"] = local_smsbuffer;
		doc["number"] = n;
		String jsonStr;
		serializeJson(doc, jsonStr);
    publish_system_state(jsonStr.c_str(),"info/sms",true);
}

void publish_json_to_mqtt(const char* jsonStr){
  
    char topic[50];
    memset(topic,'\0',50);
    sprintf_P(topic,PSTR("blackwire/%s/info/sensors"),device_id_macStr);
    if(!mqtt_enable){
#ifdef _DEBUG
      Serial.println(F("MQTT DISABLED")); 
#endif
      return;
    }
#ifdef _DEBUG
    Serial.printf_P(PSTR("MQTT - Update - topic>%s \n"), topic);
#endif
    client.publish(topic, jsonStr,true);    

}

void callback(char *topic, byte *payload, unsigned int length)
{
    /* ============================================================
       STEP 1: Validate topic belongs to this device
    ============================================================ */
    char baseTopic[100];
    sprintf_P(baseTopic, PSTR("blackwire/%s/"), device_id_macStr);

#ifdef _DEBUG
    Serial.printf_P(PSTR("MQTT - Received topic: %s\n"), topic);
#endif

    if (strstr_P(topic, PSTR("/cmd/")) == nullptr)
    {
#ifdef _DEBUG
        Serial.println(F("Ignoring: topic does not contain /cmd/"));
#endif
        return;
    }

    if (strncmp(topic, baseTopic, strlen(baseTopic)) != 0)
    {
#ifdef _DEBUG
        Serial.println(F("Ignoring: topic not for this device"));
#endif
        return;
    }

    /* ============================================================
       STEP 2: Copy payload safely
    ============================================================ */
    char payload_buffer[256];  // reduced stack usage
    unsigned int copy_length = min(length, sizeof(payload_buffer) - 1);
    memcpy(payload_buffer, payload, copy_length);
    payload_buffer[copy_length] = '\0';

#ifdef _DEBUG
    Serial.print(F("Payload: "));
    Serial.println(payload_buffer);
#endif

    /* ============================================================
       STEP 3: Parse JSON if SYSTEM COMMAND
    ============================================================ */
    StaticJsonDocument<256> jsonDoc;
    bool jsonParsed = false;
    const char* cmd = nullptr;
    uint32_t rpcId = 0;        // use uint32_t for RPC ID
    bool success = false;

    if (strstr_P(topic, PSTR("/cmd/sys/set")))
    {
#ifdef _DEBUG
        Serial.println(F("Matched: SYS SET"));
#endif
        DeserializationError err = deserializeJson(jsonDoc, payload_buffer);
        if (err)
        {
#ifdef _DEBUG
            Serial.print(F("JSON Error: "));
            Serial.println(err.c_str());
#endif
        }
        else
        {
            jsonParsed = true;

            if (jsonDoc.containsKey("cmd"))
            {
                cmd = jsonDoc["cmd"];
                rpcId = jsonDoc["rpcId"] | 0;  // default 0 if not provided

                /* ---------------- ALARM ---------------- */
                if (strcmp_P(cmd, PSTR("alarm")) == 0)
                {
#ifdef _DEBUG
                    Serial.println(F("Matched: alarm"));
#endif
                    transfer_mqtt_data("Alarm_call");
                    success = true;
                }

                /* ---------------- MODE ---------------- */
                else if (strcmp_P(cmd, PSTR("mod")) == 0 &&
                         jsonDoc["data"].containsKey("mod"))
                {
#ifdef _DEBUG
                    Serial.println(F("Matched: mod"));
#endif
                    const char* mod = jsonDoc["data"]["mod"];
                    if (strcmp_P(mod, PSTR("a")) == 0)
                    {
#ifdef _DEBUG
                        Serial.println(F("Mode: ARM"));
#endif
                        transfer_mqtt_data("Home arm");
                        publish_system_state("ARMED", "info/mode", true);
                        success = true;
                    }
                    else if (strcmp_P(mod, PSTR("d")) == 0)
                    {
#ifdef _DEBUG
                        Serial.println(F("Mode: DISARM"));
#endif
                        transfer_mqtt_data("Disarm");
                        publish_system_state("DISARMED", "info/mode", true);
                        success = true;
                    }
                }

                /* ---------------- SIREN ---------------- */
                else if (strcmp_P(cmd, PSTR("siren")) == 0)
                {
#ifdef _DEBUG
                    Serial.println(F("Matched: siren"));
#endif
                    bool state = jsonDoc["data"]["ste"] | false;
                    char cmdBuff[20];
                    sprintf_P(cmdBuff, PSTR("siren=%d"), state);
                    transfer_mqtt_data(cmdBuff);
                    success = true;
                }

                /* ---------------- SMS ---------------- */
                else if (strcmp_P(cmd, PSTR("sms")) == 0)
                {
#ifdef _DEBUG
                    Serial.println(F("Matched: sms"));
#endif
                    const char* msg = jsonDoc["data"]["msg"];
                    const char* number = jsonDoc["data"]["tp"];
                    creatSMS(msg, 4, number);
                    success = true;
                }
            }
        }
    }

    /* ============================================================
       STEP 4: RELAY COMMANDS
    ============================================================ */
    else if (strstr_P(topic, PSTR("/cmd/relay1/set")))
    {
#ifdef _DEBUG
        Serial.println(F("Matched: relay1"));
#endif
        if (strcmp_P(payload_buffer, PSTR("on")) == 0)
        {
#ifdef _DEBUG
            Serial.println(F("Relay1 ON"));
#endif
            transfer_mqtt_data("Relay 1 on");
        }
        else if (strcmp_P(payload_buffer, PSTR("off")) == 0)
        {
#ifdef _DEBUG
            Serial.println(F("Relay1 OFF"));
#endif
            transfer_mqtt_data("Relay 1 off");
        }
    }

    else if (strstr_P(topic, PSTR("/cmd/relay2/set")))
    {
#ifdef _DEBUG
        Serial.println(F("Matched: relay2"));
#endif
        if (strcmp_P(payload_buffer, PSTR("on")) == 0)
        {
#ifdef _DEBUG
            Serial.println(F("Relay2 ON"));
#endif
            transfer_mqtt_data("Relay 2 on");
        }
        else if (strcmp_P(payload_buffer, PSTR("off")) == 0)
        {
#ifdef _DEBUG
            Serial.println(F("Relay2 OFF"));
#endif
            transfer_mqtt_data("Relay 2 off");
        }
    }

    /* ============================================================
       STEP 5: OTA UPDATES
    ============================================================ */
    else if (strstr_P(topic, PSTR("/cmd/sys/ota/firmware")))
    {
#ifdef _DEBUG
        Serial.println(F("Matched: OTA firmware"));
#endif
        client.disconnect();
        delay(1000);
        downloadAndApplyFirmware(payload_buffer);
    }
    else if (strstr_P(topic, PSTR("/cmd/sys/ota/spiffs")))
    {
#ifdef _DEBUG
        Serial.println(F("Matched: OTA spiffs"));
#endif
        downloadAndApplySPIFFS(payload_buffer);
    }

    /* ============================================================
       STEP 6: SEND RPC RESPONSE AT THE END
    ============================================================ */
    if (jsonParsed && rpcId > 0)
    {
#ifdef _DEBUG
        Serial.println(F("Sending RPC response"));
#endif
        char responseTopic[120];
        sprintf_P(responseTopic,
                  PSTR("blackwire/%s/rpc/response"),
                  device_id_macStr);

        StaticJsonDocument<192> respDoc;
        respDoc["rpcId"] = rpcId;
        respDoc["cmd"] = cmd;
        respDoc["success"] = success;
        respDoc["ts"] = millis();

        char respBuffer[192];
        serializeJson(respDoc, respBuffer);

        client.publish(responseTopic, respBuffer);
#ifdef _DEBUG
        Serial.printf_P(PSTR("RPC Response published to %s: %s\n"), responseTopic, respBuffer);
#endif
        
    }
}










void transfer_mqtt_data(const char* msg){
	 /* keep the status of sending data */
	 BaseType_t xStatus;
	 /* time to block the task until the queue has free space */
	 const TickType_t xTicksToWait = pdMS_TO_TICKS(1);
	 /* create data to send */
	DataBuffer data;
	/* sender 1 has id is 1 */
	memset(data.char_buffer_rx,'\0',15);
	strcpy(data.char_buffer_rx,msg);

	
		Serial.println(F("sendQueu_mqtt_data"));
		/* send data to front of the queue */
		xStatus = xQueueSendToFront(xQueue_mqtt_Qhdlr, &data, xTicksToWait );
		/* check whether sending is ok or not */
		if( xStatus == pdPASS ) {
			/* increase counter of sender 1 */
			;
			Serial.println(F("sendQueu_mqtt_data sending data"));
		}
		/* we delay here so that receiveTask has chance to receive data */
		delay(100);
}



void setup_subscriptions(){
  if(!mqtt_enable){Serial.println(F("MQTT DISABLED")); return;}
  // Construct the MQTT topic
  char mqttTopic[100];  // Adjust the size as needed
  sprintf_P(mqttTopic, PSTR("blackwire/%s/cmd"), device_id_macStr);
    Serial.println(F("setup subscriptions"));    
    client.subscribe(mqttTopic); // subscribe from the topic
  sprintf_P(mqttTopic, PSTR("blackwire/%s/cmd/relay1/set"), device_id_macStr);
    client.subscribe(mqttTopic); // subscribe from the topic
  sprintf_P(mqttTopic, PSTR("blackwire/%s/cmd/relay2/set"), device_id_macStr);
    client.subscribe(mqttTopic); // subscribe from the topic
  sprintf_P(mqttTopic, PSTR("blackwire/%s/cmd/sys/set"), device_id_macStr);
    client.subscribe(mqttTopic); // subscribe from the topic
  sprintf_P(mqttTopic, PSTR("blackwire/%s/cmd/sys/ota/firmware"), device_id_macStr);
    client.subscribe(mqttTopic); // subscribe from the topic
  sprintf_P(mqttTopic, PSTR("blackwire/%s/cmd/sys/ota/spiffs"), device_id_macStr);
    client.subscribe(mqttTopic); // subscribe from the topic
    Serial.println(mqttTopic);    
   }


void mqtt_com_loop() {

  if(!mqtt_enable){return;
  }

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop(); 
  delay(500);
}
