
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

void setup_mqtt(){
  
  
  if(!mqtt_enable){Serial.println(F("MQTT DISABLED")); return;}
  else{
    Serial.printf_P(PSTR("MQTT_SERVR:%s\nPORT:%d\nUSER:%s\nPASS:%s"),mqttServer,mqtt_port,mqtt_username,mqtt_password);
  }
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


void set_onMQTT_connection(_callbackFunctionType7 pFn){fn_onMQTT_connection = pFn;}


void reconnectMQTT() {
  if(!mqtt_enable){Serial.println(F("MQTT DISABLED")); return;}
  int state =WiFi.status();
  if(state !=WL_CONNECTED)
  {
      Serial.println(F("No WiFi to Reconnect MQTT"));
      return;
  }
  // creat dead message
  uint8_t mac[6];
  char device_id_macStr[18];
  WiFi.macAddress(mac);	
  // Format the MAC address without colons and with underscores
  sprintf(device_id_macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  String client_id = "blackwire-";
  client_id += String(device_id_macStr);
  Serial.printf_P(PSTR("MQTT client id: %s\n"), client_id.c_str());

  char lastwill_topic[50];
  memset(lastwill_topic,'\0',50);
  sprintf_P(lastwill_topic,PSTR("blackwire/%s/info/status"),device_id_macStr);

  while (!client.connected()) {
    vTaskDelay(5000/portTICK_PERIOD_MS);
    Serial.printf("Reconnecting to MQTT broker... at %s\n",mqttServer);
     // set root ca cert
#ifdef MQTT_SECURE
  espClient.setCACert(ca_cert);
#endif
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password,lastwill_topic, 1, true, "offline")) {
        Serial.println(F("Connected to MQTT broker."));
        client.publish(lastwill_topic,"online",true);
        publish_system_state(WiFi.localIP().toString().c_str(),"info/ip",true);
        setup_subscriptions();
        fn_onMQTT_connection();
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

void publish_network_info(){
  //long rssi = WiFi.RSSI();
  String rssi_value = String(WiFi.RSSI());
  publish_system_state(rssi_value.c_str(),"info/rssi",false);
}


void send_rfid_state_update_to_mqtt(const char* rfid){
       
   publish_system_state(rfid, "info/rfid", false);

}


void publish_system_state(const char* state, const char* subtopic, bool retaind_flag){

    //creat topic
    uint8_t mac[6];
    char device_id_macStr[18];
    WiFi.macAddress(mac);	
// Format the MAC address without colons and with underscores
   sprintf(device_id_macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
   
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

    uint8_t mac[6];
    char device_id_macStr[18];
    WiFi.macAddress(mac);	
// Format the MAC address without colons and with underscores
   sprintf(device_id_macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
   
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

void callback(char *topic, byte *payload, unsigned int length) {
#ifdef _DEBUG
    Serial.print(F("Message arrived in topic: "));
    Serial.println(topic);
#endif

    char my_topic[100];
    uint8_t mac[6];
    char device_id_macStr[18];
    WiFi.macAddress(mac);
    sprintf(device_id_macStr, "%02X%02X%02X%02X%02X%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

#ifdef _DEBUG
    String byteRead = "";
    Serial.print(F("Message: "));
    for (int i = 0; i < length; i++) {
        byteRead += (char)payload[i];
    }
    Serial.println(byteRead);
#endif

    memset(my_topic, '\0', sizeof(my_topic));
    sprintf_P(my_topic, PSTR("blackwire/%s/cmd/sys/set"), device_id_macStr);

    if (strcmp(topic, my_topic) == 0) {
        DynamicJsonDocument jsonDoc(256);
        DeserializationError err = deserializeJson(jsonDoc, payload, length);

        if (err) {
#ifdef _DEBUG
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(err.c_str());
#endif
            return;
        }

        if (!jsonDoc.containsKey("cmd")) return;

        const char* cmd = jsonDoc["cmd"];
        if (jsonDoc.containsKey("data") && jsonDoc["data"].is<JsonObject>()) {
            JsonObject data = jsonDoc["data"];

            if (strncmp(cmd, "mod", 3) == 0) {
                if (data.containsKey("mod")) {
                    const char* mod = data["mod"];
                    if (strncmp(mod, "a", 1) == 0) {
                        transfer_mqtt_data("Home arm");
                        publish_system_state("ARMED", "info/mode", true);
                    } else if (strncmp(mod, "d", 1) == 0) {
                        transfer_mqtt_data("Disarm");
                        publish_system_state("DISARMED", "info/mode", true);
                    }
                }
            }

            if (strncmp(cmd, "sms", 3) == 0) {
                if (data.containsKey("msg") && data.containsKey("tp")) {
                    const char* data_msg = data["msg"];
                    const char* data_number = data["tp"];
                    uint8_t data_type = 4;
                    creatSMS(data_msg, data_type, data_number);
                }
            }

            if (strncmp(cmd, "siren", 5) == 0) {
                int data_duration = data.containsKey("tm") ? data["tm"] : 0;
                bool data_state = data.containsKey("ste") ? data["ste"] : false;
                char cmdBuff[20] = "";
                sprintf_P(cmdBuff, PSTR("siren=%d"), data_state);
                transfer_mqtt_data(cmdBuff);
            }
        }

        if (strncmp(cmd, "alarm", 5) == 0) {
            transfer_mqtt_data("Alarm_call");
        } else if (strncmp(cmd, "chime1", 6) == 0) {
            transfer_mqtt_data(cmd);
        }
    }

    // Relay1
    memset(my_topic, '\0', sizeof(my_topic));
    sprintf_P(my_topic, PSTR("blackwire/%s/cmd/relay1/set"), device_id_macStr);

    if (strcmp(topic, my_topic) == 0) {
        char payload_buffer[50];
        unsigned int copy_length = min(length, sizeof(payload_buffer) - 1);
        memcpy(payload_buffer, payload, copy_length);
        payload_buffer[copy_length] = '\0';

        if (strcmp(payload_buffer, "on") == 0) {
            transfer_mqtt_data("Relay 1 on");
        } else if (strcmp(payload_buffer, "off") == 0) {
            transfer_mqtt_data("Relay 1 off");
        }
    }

    // Relay2
    memset(my_topic, '\0', sizeof(my_topic));
    sprintf_P(my_topic, PSTR("blackwire/%s/cmd/relay2/set"), device_id_macStr);

    if (strcmp(topic, my_topic) == 0) {
        char payload_buffer[50];
        unsigned int copy_length = min(length, sizeof(payload_buffer) - 1);
        memcpy(payload_buffer, payload, copy_length);
        payload_buffer[copy_length] = '\0';

        if (strcmp(payload_buffer, "on") == 0) {
            transfer_mqtt_data("Relay 2 on");
        } else if (strcmp(payload_buffer, "off") == 0) {
            transfer_mqtt_data("Relay 2 off");
        }
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
  uint8_t mac[6];
  char device_id_macStr[18];
  WiFi.macAddress(mac);	
// Format the MAC address without colons and with underscores
  sprintf(device_id_macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
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
