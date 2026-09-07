
#include "mqtt_broker.h"
#include "siren.h"   // g_siren_master_disabled + EventRTOS_siren
#include "pinsx.h"      // FW_VER + board select, reported in the boot message
#include <esp_system.h> // esp_reset_reason()

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

// PubSubClient is single-threaded: `client` may only be touched by ONE task.
// Normally publishes are short and rare enough that the pre-existing multi-task
// publishing is tolerated, but a chunked OTA hammers client.loop()+publish() on
// the MQTT task for the whole (minutes-long) download. A concurrent publish from
// any other task (alarm/GSM/sensor/web) would corrupt PubSubClient's shared
// buffer -> a malformed packet (broker drops us) AND corrupted chunk RX bytes
// (sha_mismatch). So while an OTA is active, ONLY the MQTT task may use `client`;
// every other task's telemetry publish is dropped for the download window.
TaskHandle_t g_mqtt_task = nullptr;   // captured in mqtt_com_loop()

// Cached MQTT link state, maintained ONLY by the MQTT task. Any other task that
// wants to know if MQTT is up (e.g. the websocket page builder on loopTask) MUST
// read this flag and MUST NOT call client.connected() itself — on the secure
// (WiFiClientSecure/TLS) transport, connected() does an SSL read that mutates the
// record layer, so a second task touching it corrupts the session (invalid SSL
// record -> pbuf double-free crash), especially while an OTA saturates the link.
volatile bool g_mqtt_online = false;

bool mqtt_foreign_tx_blocked(){
  return TasksOTA::active() && (xTaskGetCurrentTaskHandle() != g_mqtt_task);
}

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

// Report the master-siren-disable state as a retained attribute so the dashboard
// button reflects the device. Published on every change AND on each MQTT connect
// -> after a reboot the flag is back to false (enabled), so the button reverts.
void publish_siren_state(){
    publish_system_state(g_siren_master_disabled ? "true" : "false", "info/siren_disabled", true);
}


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
        g_mqtt_online = true;
        client.publish(lastwill_topic,"online",true);
        publish_system_state(WiFi.localIP().toString().c_str(),"info/ip",true);
        publish_siren_state();   // re-sync the master-siren-disable button (false after a reboot)
        publish_boot_info();     // one-shot per boot: fw + hardware identity
        setup_subscriptions();
        TasksOTA::begin(otaMqttPublishCb, "info/sys/ota");
        TasksOTA::resumeIfPaused();
        TasksOTA::bootReportIfNeeded();
        uint32_t colour = Adafruit_NeoPixel::Color(200, 0, 255);
  		  pixel.startBlink(colour, 100, 1000, 255);
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


// ── Boot announcement ──────────────────────────────────────────────────────
// Board identity for the boot message. Mirrors the board selects in pinsx.h;
// kept next to the telemetry so flipping the board target needs no edit here
// beyond adding a new case.
#if   defined(GSM_MINI_BOARD_V3)
  #define BW_BOARD_NAME "GSM_MINI_BOARD_V3"
#elif defined(GSM_MINI_BOARD_V2)
  #define BW_BOARD_NAME "GSM_MINI_BOARD_V2"
#elif defined(GSM_PULSEX_IOT_BOARD)
  #define BW_BOARD_NAME "GSM_PULSEX_IOT_BOARD"
#elif defined(GSM_FULL_BOARD)
  #define BW_BOARD_NAME "GSM_FULL_BOARD"
#elif defined(GSM_MINI_BOARD)
  #define BW_BOARD_NAME "GSM_MINI_BOARD"
#else
  #define BW_BOARD_NAME "unknown"
#endif

// Why the last reset happened — lets the backend tell a user power-cycle from a
// crash loop (panic / wdt) or an OTA reboot (sw).
static const char* reset_reason_str(){
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:   return "poweron";
    case ESP_RST_EXT:       return "ext";
    case ESP_RST_SW:        return "sw";
    case ESP_RST_PANIC:     return "panic";
    case ESP_RST_INT_WDT:   return "int_wdt";
    case ESP_RST_TASK_WDT:  return "task_wdt";
    case ESP_RST_WDT:       return "wdt";
    case ESP_RST_DEEPSLEEP: return "deepsleep";
    case ESP_RST_BROWNOUT:  return "brownout";
    case ESP_RST_SDIO:      return "sdio";
    default:                return "unknown";
  }
}

// One-shot per boot: latched only on a successful publish, so a send that loses
// the race with a dropping link is retried on the next reconnect. Plain RAM, so
// every reset (OTA reboot included) re-arms it.
static bool s_boot_info_sent = false;

// Announce firmware + hardware identity once per boot on info/sys/boot.
// RETAINED: the backend gets the device's identity even if it subscribes long
// after the device came up, and the broker keeps exactly one current copy.
// Called from reconnectMQTT() on the MQTT task — never call it from elsewhere,
// nothing but that task may touch `client`.
void publish_boot_info(){
  if (s_boot_info_sent) return;
  if (!mqtt_enable){
    #ifdef _DEBUG
      Serial.println(F("MQTT DISABLED"));
    #endif
    return;
  }
  if (mqtt_foreign_tx_blocked()) return;

  uint8_t mac[6];
  char device_id_macStr[13];
  WiFi.macAddress(mac);
  sprintf(device_id_macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  char topic[50];
  memset(topic,'\0',50);
  sprintf_P(topic,PSTR("blackwire/%s/info/sys/boot"),device_id_macStr);

  DynamicJsonDocument doc(512);
  doc["ev"]          = "boot";
  doc["fw"]          = FW_VER;
  doc["board"]       = BW_BOARD_NAME;
  doc["mac"]         = device_id_macStr;
  doc["chip"]        = ESP.getChipModel();
  doc["rev"]         = ESP.getChipRevision();
  doc["cores"]       = ESP.getChipCores();
  doc["cpu_mhz"]     = ESP.getCpuFreqMHz();
  doc["flash_kb"]    = ESP.getFlashChipSize() / 1024;
  doc["sdk"]         = ESP.getSdkVersion();
  doc["app_kb"]      = ESP.getSketchSize() / 1024;        // running image size
  doc["ota_free_kb"] = ESP.getFreeSketchSpace() / 1024;   // room for the next OTA
  doc["heap_kb"]     = ESP.getFreeHeap() / 1024;
  doc["reset"]       = reset_reason_str();
  doc["ip"]          = WiFi.localIP().toString();
  doc["rssi"]        = WiFi.RSSI();

  String jsonStr;
  serializeJson(doc, jsonStr);
  #ifdef _DEBUG
    Serial.printf_P(PSTR("MQTT - BOOT - topic>%s payload>%s\n"), topic, jsonStr.c_str());
  #endif
  s_boot_info_sent = client.publish(topic, jsonStr.c_str(), true);
}

void publish_system_state(const char* state, const char* subtopic, bool retaind_flag){

    // During OTA only the MQTT task may touch `client` (see mqtt_foreign_tx_blocked).
    if (mqtt_foreign_tx_blocked()) return;

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

// Report the outcome of an outgoing SMS to TB (via the bridge on info/sms/sent).
// ok=true means the modem accepted it for sending (AT+CMGS OK) — NOT a delivery
// receipt. Called from the SMS task; publish_system_state carries the OTA guard.
void publish_sms_result(bool ok, const char* number, const char* msg, uint8_t type){
  DynamicJsonDocument doc(300);
		doc["status"] = ok ? "sent" : "failed";
		doc["number"] = number;
		doc["msg"]    = msg;
		doc["type"]   = type;
		String jsonStr;
		serializeJson(doc, jsonStr);
    publish_system_state(jsonStr.c_str(),"info/sms/sent",false);
}

// Report the outcome of an alarm voice call to TB (via the bridge on
// info/call/result). status is one of answered|busy|no_answer|failed; number is
// the contact dialed, slot is its contact index. Called from the call task.
void publish_call_result(const char* status, const char* number, int slot){
  DynamicJsonDocument doc(200);
		doc["status"] = status;
		doc["number"] = number;
		doc["slot"]   = slot;
		String jsonStr;
		serializeJson(doc, jsonStr);
    publish_system_state(jsonStr.c_str(),"info/call/result",false);
}

void publish_json_to_mqtt(const char* jsonStr){

    // During OTA only the MQTT task may touch `client` (see mqtt_foreign_tx_blocked).
    if (mqtt_foreign_tx_blocked()) return;

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

// ── Two-way RPC channel ─────────────────────────────────────────────────────
// All device ACTIONS arrive as ThingsBoard two-way RPCs relayed by the Node-RED
// bridge on  blackwire/<MAC>/rpc/req , and are answered on  blackwire/<MAC>/rpc/res .
// Request envelope (nested, as AgroFlow delivers it; flat is also accepted):
//   {"device":"blackwire_<MAC>","data":{"id":<int>,"method":"<name>","params":<obj>}}
// Response:
//   success: {"reqId":<id>,"success":true,"result":<obj>}   (result optional)
//   failure: {"reqId":<id>,"success":false,"error":"<code>"}
// Long actions (OTA) ack {"status":"started"} immediately, then run async.
static void rpc_reply_ok(int reqId, const char* result_json) {
    char res[128];
    if (result_json && result_json[0])
        snprintf(res, sizeof(res), "{\"reqId\":%d,\"success\":true,\"result\":%s}", reqId, result_json);
    else
        snprintf(res, sizeof(res), "{\"reqId\":%d,\"success\":true}", reqId);
    publish_system_state(res, "rpc/res", false);
}

static void rpc_reply_err(int reqId, const char* code) {
    char res[96];
    snprintf(res, sizeof(res), "{\"reqId\":%d,\"success\":false,\"error\":\"%s\"}", reqId, code);
    publish_system_state(res, "rpc/res", false);
}

// Build + publish the full contact list (all 8 slots). Too big for
// rpc_reply_ok's fixed buffer, so it assembles the whole response with
// ArduinoJson. Reads personx.json once. call/sms are read the same way the
// getters do, so the reported flags match how the device actually uses them.
static void rpc_reply_contacts(int reqId) {
    DynamicJsonDocument src(2048);
    File f = SPIFFS.open("/personx.json", FILE_READ);
    if (f) { deserializeJson(src, f); f.close(); }

    DynamicJsonDocument out(2048);
    out["reqId"]   = reqId;
    out["success"] = true;
    JsonObject result = out.createNestedObject("result");
    JsonArray  arr    = result.createNestedArray("contacts");
    for (int i = 1; i <= 8; i++) {
        char key[8];
        snprintf(key, sizeof(key), "P%d", i);
        JsonObject c = arr.createNestedObject();
        c["slot"]   = i;
        c["number"] = src[key]["number"] | "N";
        c["call"]   = (bool)src[key]["call"];
        c["sms"]    = (bool)src[key]["sms"];
        c["remID"]  = src[key]["remID"] | "";   // bound remote keyfob, "" = none
    }
    String s;
    serializeJson(out, s);
    publish_system_state(s.c_str(), "rpc/res", false);
}

// Zone attributes live in /zone_data_8.json as z00..z07 (this board = 8 zones).
// Sending all zones' full params in one RPC is wasteful, so the dashboard
// selects one zone and we read/patch just that zone (name + flags) here.
#define ZONE_FILE  "/zone_data_8.json"
#define ZONE_COUNT 8
#define ZONE_NAME_MAX 10   // device_name[11] in typex.h -> 10 chars + NUL

// Copy the flag/name fields of one zone object into the RPC result. Shared by
// rpc_reply_zone (single) so the set-echo and the get return the same shape.
static void zone_fill_result(JsonObject r, JsonObject z, int zone) {
    r["zone"] = zone;
    r["n"]   = z["n"] | "";
    r["by"]  = (bool)z["by"];   // bypass
    r["ed"]  = (bool)z["ed"];   // entry delay
    r["xd"]  = (bool)z["xd"];   // exit delay
    r["x24"] = (bool)z["x24"];  // 24-hour
    r["pm"]  = (bool)z["pm"];   // perimeter
    r["sl"]  = (bool)z["sl"];   // silent / chime
    r["rf"]  = (bool)z["rf"];   // RF sensor
}

// Read one zone from SPIFFS and publish its full params on rpc/res.
static void rpc_reply_zone(int reqId, int zone) {
    DynamicJsonDocument src(JSON_DOC_SIZE_ZONE_DATA);
    File f = SPIFFS.open(ZONE_FILE, FILE_READ);
    if (f) { deserializeJson(src, f); f.close(); }
    char zk[6];
    snprintf(zk, sizeof(zk), "z%02d", zone);

    DynamicJsonDocument out(512);
    out["reqId"]   = reqId;
    out["success"] = true;
    JsonObject r = out.createNestedObject("result");
    zone_fill_result(r, src[zk].as<JsonObject>(), zone);
    String s;
    serializeJson(out, s);
    publish_system_state(s.c_str(), "rpc/res", false);
}

// Lightweight list — zone number + name only — so the dashboard can populate a
// zone selector without pulling every zone's full params at once.
static void rpc_reply_zones(int reqId) {
    DynamicJsonDocument src(JSON_DOC_SIZE_ZONE_DATA);
    File f = SPIFFS.open(ZONE_FILE, FILE_READ);
    if (f) { deserializeJson(src, f); f.close(); }

    DynamicJsonDocument out(1024);
    out["reqId"]   = reqId;
    out["success"] = true;
    JsonObject r   = out.createNestedObject("result");
    JsonArray  arr = r.createNestedArray("zones");
    for (int i = 0; i < ZONE_COUNT; i++) {
        char zk[6];
        snprintf(zk, sizeof(zk), "z%02d", i);
        JsonObject o = arr.createNestedObject();
        o["zone"] = i;
        o["n"]    = src[zk]["n"] | "";
    }
    String s;
    serializeJson(out, s);
    publish_system_state(s.c_str(), "rpc/res", false);
}

// RF (wireless sensor/remote) IDs are stored per-zone in /rfid.json as z00..zNN
// -> 10-char code strings. Same select-a-zone pattern as the zone attributes.
#define RFID_FILE "/rfid.json"
#define RFID_MAX  10   // matches the fixed 10-char codes already stored

// Read one zone's RF ID and publish it on rpc/res. Reads the file directly
// rather than via get_device_RFID() to avoid its fixed 10-byte return buffer.
static void rpc_reply_rfid(int reqId, int zone) {
    DynamicJsonDocument src(JSON_DOC_SIZE_ZONE_DATA);
    File f = SPIFFS.open(RFID_FILE, FILE_READ);
    if (f) { deserializeJson(src, f); f.close(); }
    char zk[6];
    snprintf(zk, sizeof(zk), "z%02d", zone);
    const char* id = src[zk] | "";
    char result[48];
    snprintf(result, sizeof(result), "{\"zone\":%d,\"rfid\":\"%s\"}", zone, id);
    rpc_reply_ok(reqId, result);
}

// Remote (keyfob) IDs are bound to a person slot (1..8) in /personx.json under
// "remID" -> 10-char code. Get/set one remote at a time (the dashboard reuses
// the contact slot selector; contacts_get already reports each slot's remID).
#define REMOTE_MAX 10   // remote_rf_id[10] in typex.h

static void rpc_reply_remote(int reqId, int slot) {
    DynamicJsonDocument src(JSON_DOC_SIZE_USER_DATA);
    File f = SPIFFS.open("/personx.json", FILE_READ);
    if (f) { deserializeJson(src, f); f.close(); }
    char key[8];
    snprintf(key, sizeof(key), "P%d", slot);
    const char* id = src[key]["remID"] | "";
    char result[48];
    snprintf(result, sizeof(result), "{\"slot\":%d,\"remID\":\"%s\"}", slot, id);
    rpc_reply_ok(reqId, result);
}

static void handle_rpc(byte* payload, unsigned int length) {
    DynamicJsonDocument doc(512);
    if (deserializeJson(doc, payload, length)) {
        Serial.println(F("RPC: JSON parse error"));
        return;
    }
    // Accept both the nested {"data":{...}} form and a flattened {...} form.
    JsonObject data   = doc.containsKey("data") ? doc["data"].as<JsonObject>() : doc.as<JsonObject>();
    const char* method = data["method"];
    JsonObject params  = data["params"].as<JsonObject>();

    if (!data.containsKey("id") || method == nullptr) {
        Serial.println(F("RPC: ignored - missing id or method"));
        return;
    }
    int reqId = data["id"].as<int>();
    Serial.printf("RPC: id=%d method=%s\n", reqId, method);

    if (strcmp(method, "arm_set") == 0) {
        const char* state = params["state"] | "";
        if (strcmp(state, "arm") == 0) {
            transfer_mqtt_data("Home arm");
            publish_system_state("ARMED", "info/mode", true);
            rpc_reply_ok(reqId, "{\"state\":\"armed\"}");
        } else if (strcmp(state, "disarm") == 0) {
            transfer_mqtt_data("Disarm");
            publish_system_state("DISARMED", "info/mode", true);
            rpc_reply_ok(reqId, "{\"state\":\"disarmed\"}");
        } else {
            rpc_reply_err(reqId, "bad_state");
        }

    } else if (strcmp(method, "relay_set") == 0) {
#ifdef GSM_MINI_BOARD_V3
        int relay = params["relay"] | 0;
        const char* state = params["state"] | "";
        bool on  = (strcmp(state, "on")  == 0);
        bool off = (strcmp(state, "off") == 0);
        if ((relay == 1 || relay == 2) && (on || off)) {
            char buf[20];
            snprintf(buf, sizeof(buf), "Relay %d %s", relay, on ? "on" : "off");
            transfer_mqtt_data(buf);   // status readback is published on cmd/relay<N>/status
            char result[40];
            snprintf(result, sizeof(result), "{\"relay\":%d,\"state\":\"%s\"}", relay, on ? "on" : "off");
            rpc_reply_ok(reqId, result);
        } else {
            rpc_reply_err(reqId, "bad_params");
        }
#else
        // No auxiliary relays wired on this board (e.g. GSM_PULSEX_IOT_BOARD).
        rpc_reply_err(reqId, "not_supported");
#endif

    } else if (strcmp(method, "siren_set") == 0) {
        const char* state = params["state"] | "";
        bool on = (strcmp(state, "on") == 0);
        char cmdBuff[20];
        snprintf(cmdBuff, sizeof(cmdBuff), "siren=%d", on ? 1 : 0);
        transfer_mqtt_data(cmdBuff);
        char result[24];
        snprintf(result, sizeof(result), "{\"state\":\"%s\"}", on ? "on" : "off");
        rpc_reply_ok(reqId, result);

    } else if (strcmp(method, "siren_master_disable") == 0) {
        // Master override: when disabled, an alarm never sounds the siren relay
        // (calls/SMS/buzzer still fire). RAM-only — auto-clears on reboot, so the
        // device re-publishes the state on connect and the dashboard button
        // reverts to enabled after a power cycle.
        bool disable = params["disable"] | false;
        g_siren_master_disabled = disable;
        if (disable) {
            xEventGroupSetBits(EventRTOS_siren, TASK_1_BIT);  // silence any active siren now
        }
        publish_siren_state();
        char result[24];
        snprintf(result, sizeof(result), "{\"disabled\":%s}", disable ? "true" : "false");
        rpc_reply_ok(reqId, result);

    } else if (strcmp(method, "sms_send") == 0) {
        const char* tp  = params["tp"]  | "";
        const char* msg = params["msg"] | "";
        if (!tp[0] || !msg[0]) {
            rpc_reply_err(reqId, "bad_params");
        } else if (!phone_number_validat(tp)) {
            // Reject up front — otherwise the SMS task silently drops a bad
            // number after we've already replied "queued".
            rpc_reply_err(reqId, "bad_number");
        } else {
            creatSMS(msg, 4, tp);   // type 4 = send to this one number
            rpc_reply_ok(reqId, "{\"status\":\"queued\"}");
        }

    } else if (strcmp(method, "alarm_trigger") == 0) {
        transfer_mqtt_data("Alarm_call");
        rpc_reply_ok(reqId, "{\"status\":\"started\"}");

    } else if (strcmp(method, "call_test") == 0) {
        // Ring one arbitrary number once, reusing the alarm call pipeline (which
        // pauses the SMS task before any ATD). Rejected if an alarm is dialing or
        // any call is already up. Long action -> ack "calling" now; the outcome
        // arrives on info/call/result. The call task issues the ATD, not us.
        const char* number = params["number"] | "";
        EventBits_t gsmBits = xEventGroupGetBits(EventRTOS_gsm);
        bool alarm_calling = (myAlarm_pannel.get_system_state() == ALARM_CALLING);
        bool call_busy = (gsmBits & TASK_6_BIT) || (eCurruntGSM_state != GSM_LISTIN) || g_test_call_mode;
        if (!number[0] || !phone_number_validat(number)) {
            rpc_reply_err(reqId, "bad_number");
        } else if (alarm_calling) {
            rpc_reply_err(reqId, "alarm_active");   // never interrupt a real alarm call
        } else if (call_busy) {
            rpc_reply_err(reqId, "call_busy");
        } else if (!systemConfig.call_en) {
            rpc_reply_err(reqId, "call_disabled");
        } else {
            strlcpy(g_test_call_number, number, sizeof(g_test_call_number));
            g_test_call_mode = true;
            xEventGroupSetBits(EventRTOS_gsm, TASK_1_BIT);    // same trigger the alarm uses
            xEventGroupClearBits(EventRTOS_gsm, TASK_2_BIT);
            char result[48];
            snprintf(result, sizeof(result), "{\"status\":\"calling\",\"number\":\"%s\"}", number);
            rpc_reply_ok(reqId, result);
        }

    } else if (strcmp(method, "chime") == 0) {
        transfer_mqtt_data("chime1");
        rpc_reply_ok(reqId, nullptr);

    } else if (strcmp(method, "contact_set") == 0) {
        int slot = params["slot"] | 0;
        const char* number = params["number"] | "";
        if (slot < 1 || slot > 8) {
            rpc_reply_err(reqId, "bad_slot");
        } else if (!phone_number_validat(number)) {
            rpc_reply_err(reqId, "bad_number");
        } else {
            set_GSM_number((uint8_t)slot, number);
            // Optional flags: enable/disable call and/or SMS for this slot too.
            if (params.containsKey("call")) set_GSM_number_is_call((uint8_t)slot, params["call"].as<bool>());
            if (params.containsKey("sms"))  set_GSM_number_is_sms((uint8_t)slot, params["sms"].as<bool>());
            char result[64];
            snprintf(result, sizeof(result), "{\"slot\":%d,\"number\":\"%s\"}", slot, number);
            rpc_reply_ok(reqId, result);
        }

    } else if (strcmp(method, "contacts_get") == 0) {
        rpc_reply_contacts(reqId);

    } else if (strcmp(method, "contact_clear") == 0) {
        int slot = params["slot"] | 0;
        if (slot < 1 || slot > 8) {
            rpc_reply_err(reqId, "bad_slot");
        } else {
            set_GSM_number((uint8_t)slot, "N");          // "N" = empty-slot sentinel
            set_GSM_number_is_call((uint8_t)slot, false);
            set_GSM_number_is_sms((uint8_t)slot, false);
            char result[32];
            snprintf(result, sizeof(result), "{\"slot\":%d,\"cleared\":true}", slot);
            rpc_reply_ok(reqId, result);
        }

    } else if (strcmp(method, "zones_get") == 0) {
        // Zone selector list (number + name only).
        rpc_reply_zones(reqId);

    } else if (strcmp(method, "zone_get") == 0) {
        int zone = params["zone"] | -1;
        if (zone < 0 || zone >= ZONE_COUNT) rpc_reply_err(reqId, "bad_zone");
        else rpc_reply_zone(reqId, zone);

    } else if (strcmp(method, "zone_set") == 0) {
        int zone = params["zone"] | -1;
        const char* nm = params["n"] | "";
        if (zone < 0 || zone >= ZONE_COUNT) {
            rpc_reply_err(reqId, "bad_zone");
        } else if (params.containsKey("n") && strlen(nm) > ZONE_NAME_MAX) {
            rpc_reply_err(reqId, "bad_name");
        } else {
            // Read-modify-write the single zone file; only the keys present in
            // params are changed (partial update), the rest are preserved.
            DynamicJsonDocument zdoc(JSON_DOC_SIZE_ZONE_DATA);
            File f = SPIFFS.open(ZONE_FILE, FILE_READ);
            if (f) { deserializeJson(zdoc, f); f.close(); }
            char zk[6];
            snprintf(zk, sizeof(zk), "z%02d", zone);
            JsonObject z = zdoc[zk];
            if (params.containsKey("n"))   z["n"]   = nm;
            if (params.containsKey("by"))  z["by"]  = params["by"].as<bool>();
            if (params.containsKey("ed"))  z["ed"]  = params["ed"].as<bool>();
            if (params.containsKey("xd"))  z["xd"]  = params["xd"].as<bool>();
            if (params.containsKey("x24")) z["x24"] = params["x24"].as<bool>();
            if (params.containsKey("pm"))  z["pm"]  = params["pm"].as<bool>();
            if (params.containsKey("sl"))  z["sl"]  = params["sl"].as<bool>();
            if (params.containsKey("rf"))  z["rf"]  = params["rf"].as<bool>();
            File fw = SPIFFS.open(ZONE_FILE, FILE_WRITE);
            serializeJson(zdoc, fw);
            fw.close();
            load_zones(ZONE_FILE, 0, ZONE_COUNT);  // push new flags to live sensor array
            rpc_reply_zone(reqId, zone);           // echo back the stored state
        }

    } else if (strcmp(method, "rfid_get") == 0) {
        int zone = params["zone"] | -1;
        if (zone < 0 || zone >= ZONE_COUNT) rpc_reply_err(reqId, "bad_zone");
        else rpc_reply_rfid(reqId, zone);

    } else if (strcmp(method, "rfid_set") == 0) {
        int zone = params["zone"] | -1;
        const char* rfid = params["rfid"] | "";
        if (zone < 0 || zone >= ZONE_COUNT) {
            rpc_reply_err(reqId, "bad_zone");
        } else if (!rfid[0] || strlen(rfid) > RFID_MAX) {
            rpc_reply_err(reqId, "bad_rfid");
        } else {
            set_device_RFID((uint8_t)zone, rfid);  // writes /rfid.json
            rpc_reply_rfid(reqId, zone);           // echo back the stored id
        }

    } else if (strcmp(method, "remote_get") == 0) {
        int slot = params["slot"] | 0;
        if (slot < 1 || slot > 8) rpc_reply_err(reqId, "bad_slot");
        else rpc_reply_remote(reqId, slot);

    } else if (strcmp(method, "remote_set") == 0) {
        int slot = params["slot"] | 0;
        const char* remID = params["remID"] | "";
        if (slot < 1 || slot > 8) {
            rpc_reply_err(reqId, "bad_slot");
        } else if (!remID[0] || strlen(remID) > REMOTE_MAX) {
            rpc_reply_err(reqId, "bad_remid");
        } else {
            set_remote_RFID((uint8_t)slot, remID);  // writes /personx.json
            rpc_reply_remote(reqId, slot);          // echo back the stored id
        }

    } else if (strcmp(method, "ota_mqtt") == 0 || strcmp(method, "ota_mqtt_fs") == 0) {
        bool is_fs = (strcmp(method, "ota_mqtt_fs") == 0);
        if (TasksOTA::active()) {
            rpc_reply_err(reqId, "ota_in_progress");
            Serial.println(F("RPC: OTA ignored - already running/paused"));
            return;
        }
        // ACK "started" now (TB RPC caller times out ~8 s); service() runs the
        // chunked download on this task after this callback returns.
        TasksOTA::request(is_fs);
        rpc_reply_ok(reqId, "{\"status\":\"started\"}");
        Serial.printf("RPC: OTA %s started\n", is_fs ? "fs" : "fw");

    } else {
        Serial.printf("RPC: unknown method '%s'\n", method);
        rpc_reply_err(reqId, "unknown_method");
    }
}

void callback(char *topic, byte *payload, unsigned int length) {
#ifdef _DEBUG
    Serial.print(F("Message arrived in topic: "));
    Serial.println(topic);
#endif

    // OTA meta/chunk responses bypass the normal cmd dispatch below.
    if (TasksOTA::consume(topic, payload, length)) {
        return;
    }

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

    // All device ACTIONS arrive on the single two-way RPC channel.
    memset(my_topic, '\0', sizeof(my_topic));
    sprintf_P(my_topic, PSTR("blackwire/%s/rpc/req"), device_id_macStr);

    if (strcmp(topic, my_topic) == 0) {
        handle_rpc(payload, length);
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
    Serial.println(F("setup subscriptions"));
  // All device ACTIONS arrive on the single two-way RPC channel (arm, relay,
  // siren, sms, alarm, chime, ota_mqtt, ota_mqtt_fs). The old scattered
  // cmd/* command topics are retired — the Node-RED bridge now delivers
  // everything as ThingsBoard RPCs here.
  sprintf_P(mqttTopic, PSTR("blackwire/%s/rpc/req"), device_id_macStr);
    client.subscribe(mqttTopic, 1);
  // Chunked OTA protocol responses (meta + chunk), firmware and fs subtrees.
  sprintf_P(mqttTopic, PSTR("blackwire/%s/ota/meta/res"), device_id_macStr);
    client.subscribe(mqttTopic, 1);
  sprintf_P(mqttTopic, PSTR("blackwire/%s/ota/chunk/res/+"), device_id_macStr);
    client.subscribe(mqttTopic, 1);
  sprintf_P(mqttTopic, PSTR("blackwire/%s/otafs/meta/res"), device_id_macStr);
    client.subscribe(mqttTopic, 1);
  sprintf_P(mqttTopic, PSTR("blackwire/%s/otafs/chunk/res/+"), device_id_macStr);
    client.subscribe(mqttTopic, 1);
    Serial.println(mqttTopic);
   }


void mqtt_com_loop() {

  // This is the ONE task allowed to touch `client` during an OTA. Capture its
  // handle so mqtt_foreign_tx_blocked() can drop foreign-task publishes.
  if (!g_mqtt_task) g_mqtt_task = xTaskGetCurrentTaskHandle();

  if(!mqtt_enable){return;
  }

  // Only THIS (MQTT) task may touch `client`; refresh the cached link state that
  // other tasks read instead of calling client.connected() themselves.
  bool conn = client.connected();
  g_mqtt_online = conn;
  if (!conn) {
    reconnectMQTT();
  }
  client.loop();
  // Run pending OTA work on THIS task (PubSubClient is single-threaded).
  TasksOTA::service();
  delay(500);
}

static bool otaMqttPublishCb(const char* topic, const char* payload, bool retain)
{
    (void)topic;   // not used
    (void)retain;  // not used

    if (!payload) return false;

    //publish_json_to_mqtt(payload);
    publish_system_state(payload, topic, false);
    return true;
}
