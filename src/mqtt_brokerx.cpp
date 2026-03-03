#include "mqtt_brokerx.h"

// ----------------------------------------------------------------------------
// Existing sensor status globals (kept to avoid breaking other code)
// ----------------------------------------------------------------------------
uint8_t zone;
bool state;

// ----------------------------------------------------------------------------
// MQTT client
// ----------------------------------------------------------------------------
#ifdef MQTT_SECURE
// load DigiCert Global Root CA ca_cert (keep your existing cert block if needed)
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
extern const char * ca_cert; // If you already define it in another translation unit, keep extern.
// If not, move your existing ca_cert definition into this file.
WiFiClientSecure espClient;
#else
WiFiClient espClient;
#endif

// MQTT Broker config (kept)
static char mqttServer[100]   = {0};
static char mqtt_username[100]= {0};
static char mqtt_password[100]= {0};
static uint32_t mqtt_port     = 0;

bool mqtt_enable = false;
PubSubClient client(espClient);

// ----------------------------------------------------------------------------
// Device id
// ----------------------------------------------------------------------------
static uint8_t mac[6] = {0};
char device_id_macStr[20] = {0}; // Ensure this matches your existing global if you have one elsewhere

// ----------------------------------------------------------------------------
// Callbacks
// ----------------------------------------------------------------------------
static _callbackFunctionType7 fn_onMQTT_connection    = nullptr;
static _callbackFunctionType7 fn_onMQTT_disconnection = nullptr;

void callback_onMQTT_connection(_callbackFunctionType7 pFn){ fn_onMQTT_connection = pFn; }
void callback_onMQTT_disconnection(_callbackFunctionType7 pFn){ fn_onMQTT_disconnection = pFn; }

// ----------------------------------------------------------------------------
// RX queue (already created in main.cpp as xQueue_mqtt_Qhdlr)
// ----------------------------------------------------------------------------
xQueueHandle xQueue_mqtt_Qhdlr;

// ----------------------------------------------------------------------------
// NEW: TX queue + task
// ----------------------------------------------------------------------------
QueueHandle_t xQueue_mqtt_tx = nullptr;
static void mqtt_tx_task(void*);

// Internal direct publish (only used inside mqtt_tx_task)
static void publish_system_state_direct(const char* stateStr, const char* subtopic, bool retained_flag)
{
  if (!mqtt_enable) return;
  if (!client.connected()) return;

  char topic[80];
  memset(topic, 0, sizeof(topic));
  snprintf(topic, sizeof(topic), "blackwire/%s/%s", device_id_macStr, subtopic);

#ifdef _DEBUG
  Serial.printf_P(PSTR("MQTT TX - topic>%s payload>%s retained>%u\n"), topic, stateStr, retained_flag ? 1 : 0);
#endif

  client.publish(topic, stateStr, retained_flag);
}

void mqtt_rx_init()
{
  if (xQueue_mqtt_Qhdlr != nullptr)
    return;

  xQueue_mqtt_Qhdlr = xQueueCreate(10, sizeof(DataBuffer));

  if (!xQueue_mqtt_Qhdlr) {
    Serial.println(F("MQTT RX queue create failed"));
  } else {
    Serial.println(F("MQTT RX queue initialized"));
  }
}

void mqtt_tx_init()
{
  if (xQueue_mqtt_tx != nullptr) return;

  xQueue_mqtt_tx = xQueueCreate(20, sizeof(MqttTxMsg));
  if (!xQueue_mqtt_tx) {
    Serial.println(F("MQTT TX queue create failed"));
    return;
  }

  // TX task can block, run it on core 1
  xTaskCreatePinnedToCore(mqtt_tx_task, "mqtt_tx", 4096, nullptr, 1, nullptr, 1);
}

bool mqtt_tx_enqueue(const char* subtopic, const char* payload, bool retained)
{
  if (!mqtt_enable) return false;
  if (!xQueue_mqtt_tx) return false;

  MqttTxMsg m{};
  strlcpy(m.subtopic, subtopic, sizeof(m.subtopic));
  strlcpy(m.payload, payload, sizeof(m.payload));
  m.retained = retained;

  // non-blocking (never block the caller)
  return (xQueueSendToBack(xQueue_mqtt_tx, &m, 0) == pdPASS);
}

bool mqtt_tx_enqueue_zone(uint8_t z, bool st, bool retained)
{
  char sub[MQTT_TX_SUBTOPIC_MAX];
  snprintf(sub, sizeof(sub), "sensors/z%u", z);
  return mqtt_tx_enqueue(sub, st ? "true" : "false", retained);
}

static void mqtt_tx_task(void*)
{
  MqttTxMsg m{};
  for (;;)
  {
    if (xQueueReceive(xQueue_mqtt_tx, &m, portMAX_DELAY) != pdPASS) continue;

    // If disconnected, drop (cache in your broadcast layer if you need reliability)
    if (!client.connected()) continue;

    publish_system_state_direct(m.payload, m.subtopic, m.retained);
  }
}

// ----------------------------------------------------------------------------
// Setup MQTT
// ----------------------------------------------------------------------------
void setup_mqtt()
{
  if(!mqtt_enable){
    Serial.println(F("MQTT DISABLED"));
    return;
  }

  // create id from MAC
  WiFi.macAddress(mac);
  snprintf(device_id_macStr, sizeof(device_id_macStr),
           "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

#ifdef MQTT_SECURE
  // If ca_cert is defined elsewhere, espClient.setCACert(ca_cert) will work
  espClient.setCACert(ca_cert);
#else
  // get mqtt credintial from spiffs
  if(!getJson_key_char("/config.json", "mqtt_server", mqttServer, 100)){
    Serial.println(F("MQTT CONFIG LOAD ERROR (server)"));
  }
  if(!getJson_key_char("/config.json", "mqtt_user", mqtt_username, 100)){
    Serial.println(F("MQTT CONFIG LOAD ERROR (user)"));
  }
  if(!getJson_key_char("/config.json", "mqtt_pass", mqtt_password, 100)){
    Serial.println(F("MQTT CONFIG LOAD ERROR (pass)"));
  }
  if(!getJson_key_int("/config.json", "mqtt_port", &mqtt_port)){
    Serial.println(F("MQTT CONFIG LOAD ERROR (port)"));
  }
#endif

  client.setKeepAlive(60);
  client.setServer(mqttServer, mqtt_port);
  client.setCallback(callback);

#ifdef _DEBUG
  Serial.printf_P(PSTR("MQTT setup done. MAC=%s\n"), device_id_macStr);
#endif
}

// ----------------------------------------------------------------------------
// Reconnect (kept similar to your existing code)
// ----------------------------------------------------------------------------
void reconnectMQTT()
{
  if(!mqtt_enable){
    Serial.println(F("MQTT DISABLED"));
    return;
  }

  if(WiFi.status() != WL_CONNECTED)
  {
    Serial.println(F("No WiFi to Reconnect MQTT"));
    if(fn_onMQTT_disconnection) fn_onMQTT_disconnection();
    return;
  }

  String client_id = "blackwire-";
  client_id += String(device_id_macStr);

  char lastwill_topic[60];
  memset(lastwill_topic, 0, sizeof(lastwill_topic));
  snprintf(lastwill_topic, sizeof(lastwill_topic), "blackwire/%s/info/status", device_id_macStr);

  while (!client.connected())
  {
    vTaskDelay(pdMS_TO_TICKS(5000));
    Serial.printf("Reconnecting to MQTT broker at %s\n", mqttServer);

    // NOTE: espClient.setCACert already called in setup_mqtt for secure mode

    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password, lastwill_topic, 1, true, "offline"))
    {
      Serial.println(F("Connected to MQTT broker."));
      client.publish(lastwill_topic, "online", true);

      // publish IP non-blocking
      publish_system_state(WiFi.localIP().toString().c_str(), "info/ip", true);

      setup_subscriptions();
      if(fn_onMQTT_connection) fn_onMQTT_connection();
    }
    else
    {
      Serial.print(F("Failed to reconnect to MQTT broker, rc="));
      Serial.print(client.state());
      Serial.println(F("Retrying in 5 seconds."));
      delay(5000);
    }

    if(WiFi.status() != WL_CONNECTED)
    {
      Serial.println(F("No WiFi to Reconnect MQTT"));
      return;
    }
  }
}

// ----------------------------------------------------------------------------
// Main MQTT loop (call from your periodic task)
// ----------------------------------------------------------------------------
void mqtt_com_loop()
{
  if(!mqtt_enable) return;

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
}

// ----------------------------------------------------------------------------
// Subscriptions (keep your existing topics here)
// ----------------------------------------------------------------------------
void setup_subscriptions()
{
  if(!mqtt_enable) return;
  if(!client.connected()) return;

  char topicBase[80];
  snprintf(topicBase, sizeof(topicBase), "blackwire/%s/cmd/#", device_id_macStr);
  client.subscribe(topicBase);

#ifdef _DEBUG
  Serial.printf_P(PSTR("MQTT subscribed: %s\n"), topicBase);
#endif
}

// ----------------------------------------------------------------------------
// Publish helpers (NOW: enqueue to TX queue)
// ----------------------------------------------------------------------------
void publish_system_state(const char* stateStr, const char* subtopic, bool retained_flag)
{
  if(!mqtt_enable) return;
  mqtt_tx_enqueue(subtopic, stateStr, retained_flag);
}

void send_sensor_state_update_to_mqtt(uint8_t _zone, bool _state)
{
  mqtt_tx_enqueue_zone(_zone, _state, true);
}

void send_rfid_state_update_to_mqtt(const char* rfid)
{
  publish_system_state(rfid, "info/rfid", false);
}

void publish_incomming_sms_to_mqtt(char* local_smsbuffer, char* n)
{
  DynamicJsonDocument doc(220);
  doc["msg"] = local_smsbuffer;
  doc["number"] = n;
  String jsonStr;
  serializeJson(doc, jsonStr);
  publish_system_state(jsonStr.c_str(), "info/sms", true);
}

void publish_json_to_mqtt(const char* jsonStr)
{
  // This subtopic matches your old path blackwire/<mac>/info/sensors
  publish_system_state(jsonStr, "info/sensors", true);
}

// ----------------------------------------------------------------------------
// Inbound command transfer (MQTT RX -> Alarm Task)
// ----------------------------------------------------------------------------
void transfer_mqtt_data(const char* msg)
{
  if (!xQueue_mqtt_Qhdlr) return;

  // IMPORTANT: your original DataBuffer was char[15]; that is too small for many commands.
  // We keep it compatible by truncating safely. If you expand DataBuffer size, this will carry more.
  DataBuffer data{};
  strlcpy(data.char_buffer_rx, msg, sizeof(data.char_buffer_rx));

  // non-blocking (drop if queue full)
  xQueueSendToBack(xQueue_mqtt_Qhdlr, &data, 0);
}

// ----------------------------------------------------------------------------
// MQTT callback (mostly keep your existing parsing; here we just forward raw payload)
// ----------------------------------------------------------------------------
void callback(char *topic, byte *payload, unsigned int length)
{
#ifdef _DEBUG
  Serial.printf_P(PSTR("MQTT RX topic: %s len:%u\n"), topic, length);
#endif

  // Copy payload into null-terminated buffer
  static char payload_buffer[256];
  if (length >= sizeof(payload_buffer)) length = sizeof(payload_buffer) - 1;
  memcpy(payload_buffer, payload, length);
  payload_buffer[length] = '\0';

  // Your original code parsed JSON and called transfer_mqtt_data("...") etc.
  // Keep that logic in your project if you want.
  //
  // For now, safest default: forward payload string to alarm task.
  // If you require topic-based routing, add it here.
  transfer_mqtt_data(payload_buffer);
}
