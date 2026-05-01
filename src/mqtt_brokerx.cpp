#include "mqtt_brokerx.h"
#include "utility.h"

#include <esp_heap_caps.h>
#include <time.h>

#include "ZoneManager.h"
#include "mapping/zone_map.h"

#ifdef MQTT_SECURE
#include "mqtt_secure_config.h"
WiFiClientSecure espClient;
#else
WiFiClient espClient;
#endif

void mqtt_publish_latest_attributes();
void mqtt_publish_telemetry();
void mqtt_publish_status(const char* status, const char* reason, bool retained);
void mqtt_publish_alarm_event(const char* eventName, int zone, const char* reason, const char* alarmType);

namespace {

constexpr size_t kTopicBufferSize = 96;
constexpr size_t kInboundQueueLen = 4;
constexpr uint32_t kNtpRetryMs = 10000;
static const char* kNtpServer1 = "pool.ntp.org";
static const char* kNtpServer2 = "time.nist.gov";

struct MqttInboundMsg {
  char topic[kTopicBufferSize];
  char payload[MQTT_RX_PAYLOAD_MAX];  // larger than TX — attr/res can carry big JSON
};

static char mqttServer[100] = {0};
static char mqtt_username[100] = {0};
static char mqtt_password[100] = {0};
static uint32_t mqtt_port = 0;

static uint8_t mac[6] = {0};
static char g_deviceId[32] = {0};
static QueueHandle_t xQueue_mqtt_inbound = nullptr;

static _callbackFunctionType7 fn_onMQTT_connection = nullptr;
static _callbackFunctionType7 fn_onMQTT_disconnection = nullptr;
static uint32_t g_lastNtpRequestMs = 0;

static bool tls_time_ready() {
  time_t now = time(nullptr);
  return now > 1700000000;
}

static void log_tls_time_state() {
  const time_t now = time(nullptr);
  struct tm timeinfo {};
  if (localtime_r(&now, &timeinfo) != nullptr && now > 100000) {
    char buf[32] = {0};
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Serial.printf("TLS time gate: epoch=%ld local=%s\n", (long)now, buf);
  } else {
    Serial.printf("TLS time gate: epoch=%ld\n", (long)now);
  }
}

static void log_mqtt_connect_diagnostics() {
  Serial.printf("MQTT diag: host=%s port=%lu freeHeap=%lu minFreeHeap=%lu largest8=%u wifi=%d rssi=%d\n",
                mqttServer,
                (unsigned long)mqtt_port,
                (unsigned long)ESP.getFreeHeap(),
                (unsigned long)ESP.getMinFreeHeap(),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
                (int)WiFi.status(),
                (int)WiFi.RSSI());
  log_tls_time_state();
}

static void request_wifi_time_sync_if_needed() {
  if (WiFi.status() != WL_CONNECTED) return;

  const uint32_t nowMs = millis();
  if (g_lastNtpRequestMs != 0 && (nowMs - g_lastNtpRequestMs) < kNtpRetryMs) {
    return;
  }

  configTime(0, 0, kNtpServer1, kNtpServer2);
  g_lastNtpRequestMs = nowMs;
  Serial.println(F("NTP sync requested from MQTT wait path"));
}

static uint64_t current_ts_ms() {
  struct timeval tv {};
  if (gettimeofday(&tv, nullptr) == 0 && tv.tv_sec > 1700000000) {
    return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)(tv.tv_usec / 1000ULL);
  }
  return (uint64_t)millis();
}

static bool mqtt_time_is_synced() {
  struct timeval tv {};
  return gettimeofday(&tv, nullptr) == 0 && tv.tv_sec > 1700000000;
}

static void add_timestamp_metadata(JsonDocument& doc) {
  const bool synced = mqtt_time_is_synced();
  doc["ts"] = synced ? current_ts_ms() : (uint64_t)millis();
  if (!synced) {
    doc["tsSync"] = false;
    doc["tsSource"] = "uptime_ms";
  }
}

static void build_topic(char* out, size_t outSize, const char* suffix) {
  snprintf(out, outSize, "blackwire/%s/%s", g_deviceId, suffix);
}

static const char* arm_mode_to_str(eARM_Mode mode) {
  switch (mode) {
    case AS_ITIS_BYPASS: return "AS_IT_IS_BYPASS";
    case AS_ITIS_NO_BYPASS: return "AS_IT_IS";
    case USER_SELECT:
    default: return "USER_SELECT";
  }
}

static const char* armed_state_to_str() {
  const eMain_state state = myAlarm_pannel.get_system_state();
  if (state == DEACTIVE) return "disarm";
  if (myAlarm_pannel.get_arm_mode() == AS_ITIS_BYPASS) return "away";
  return "arm";
}

static const char* alarm_state_to_str() {
  const eMain_state state = myAlarm_pannel.get_system_state();
  return (state == ALARM_CALLING || state == PANIC) ? "triggered" : "clear";
}

static bool siren_is_active() {
  if (EventRTOS_siren == nullptr) return false;
  const EventBits_t bits = xEventGroupGetBits(EventRTOS_siren);
  return (bits & TASK_2_BIT) != 0 || myAlarm_pannel.get_system_state() == ALARM_CALLING;
}

static bool compute_trouble() {
  if (!systemConfig.ac_power) return true;
  if (!gsm_available) return true;
  for (uint8_t i = 0; i < ZONE_COUNT; ++i) {
    if (!isZoneActive(i)) continue;
    if (zoneEngine.getState(i) == ZS_FAULT) {
      return true;
    }
  }
  return false;
}

static void build_sensor_pack(char* out, size_t outSize) {
  uint8_t packed[12] = {0};
  for (uint8_t zone = 0; zone < ZONE_COUNT; ++zone) {
    uint8_t stateBits;
    if (!isZoneActive(zone)) {
      stateBits = 0x03;
    } else {
      switch (zoneEngine.getState(zone)) {
        case ZS_OPEN:  stateBits = 0x01; break;
        case ZS_FAULT: stateBits = 0x02; break;
        default:       stateBits = 0x00; break; // ZS_CLOSE
      }
    }

    const uint8_t bitIndex = zone * 2;
    packed[bitIndex / 8] |= (uint8_t)(stateBits << (bitIndex % 8));
  }

  size_t offset = 0;
  for (size_t i = 0; i < sizeof(packed) && (offset + 2) < outSize; ++i) {
    offset += snprintf(out + offset, outSize - offset, "%02X", packed[i]);
  }
  out[(outSize == 0) ? 0 : (outSize - 1)] = '\0';
}

static void build_status_payload(char* out, size_t outSize, const char* status, const char* reason) {
  StaticJsonDocument<160> doc;
  add_timestamp_metadata(doc);
  doc["status"] = status;
  if (reason != nullptr && reason[0] != '\0') {
    doc["reason"] = reason;
  }
  serializeJson(doc, out, outSize);
}

static void publish_direct(const char* suffix, const char* payload, bool retained) {
  if (!mqtt_enable || !client.connected()) return;

  char topic[kTopicBufferSize];
  build_topic(topic, sizeof(topic), suffix);

#ifdef _DEBUG
  debug_print_divider();
  debug_print_timestamp();
  Serial.printf("MQTT TX  %s\n  %s\n", suffix, payload);
  debug_print_divider();
#endif
  client.publish(topic, payload, retained);
}

static bool mqtt_tx_enqueue(const char* topic, const char* payload, bool retained) {
  if (!mqtt_enable || xQueue_mqtt_tx == nullptr) return false;

  MqttTxMsg msg {};
  strlcpy(msg.topic, topic, sizeof(msg.topic));
  strlcpy(msg.payload, payload, sizeof(msg.payload));
  msg.retained = retained;
  return xQueueSendToBack(xQueue_mqtt_tx, &msg, 0) == pdPASS;
}

static void drain_mqtt_tx_queue() {
  if (xQueue_mqtt_tx == nullptr || !client.connected()) return;

  MqttTxMsg msg {};
  while (xQueueReceive(xQueue_mqtt_tx, &msg, 0) == pdPASS) {
    publish_direct(msg.topic, msg.payload, msg.retained);
  }
}

static bool mqtt_inbound_enqueue(const char* topic, const char* payload) {
  if (xQueue_mqtt_inbound == nullptr) return false;

  MqttInboundMsg msg {};
  strlcpy(msg.topic, topic, sizeof(msg.topic));
  strlcpy(msg.payload, payload, sizeof(msg.payload));
  return xQueueSendToBack(xQueue_mqtt_inbound, &msg, 0) == pdPASS;
}

static void mqtt_publish_json(const char* topic, JsonDocument& doc, bool retained) {
  char payload[MQTT_TX_PAYLOAD_MAX];
  serializeJson(doc, payload, sizeof(payload));
  mqtt_tx_enqueue(topic, payload, retained);
}

static void mqtt_publish_rpc_success(const char* reqId, JsonDocument& resultDoc) {
  StaticJsonDocument<256> doc;
  doc["reqId"] = reqId;
  doc["success"] = true;
  doc["result"] = resultDoc.as<JsonVariantConst>();
  mqtt_publish_json("rpc/res", doc, false);
}

static void mqtt_publish_rpc_success(const char* reqId, const char* key, const char* value) {
  StaticJsonDocument<96> result;
  result[key] = value;
  mqtt_publish_rpc_success(reqId, result);
}

static void mqtt_publish_rpc_failure(const char* reqId, const char* code, const char* message) {
  StaticJsonDocument<256> doc;
  doc["reqId"] = reqId;
  doc["success"] = false;
  JsonObject error = doc.createNestedObject("error");
  error["code"] = code;
  error["message"] = message;
  mqtt_publish_json("rpc/res", doc, false);
}

static int8_t zone_block_base(const char* blockKey) {
  static const struct {
    const char* key;
    uint8_t base;
  } kZoneBlocks[] = {
    {"z_atr00_07", 0}, {"z_atr08_15", 8}, {"z_atr16_23", 16},
    {"z_atr24_31", 24}, {"z_atr32_39", 32}, {"z_atr40_47", 40},
  };

  for (const auto& blk : kZoneBlocks) {
    if (strcmp(blockKey, blk.key) == 0) return (int8_t)blk.base;
  }
  return -1;
}

// Zone keys inside each block use absolute 0-based numbering.
// Example: z_atr40_47.z40 => internal zone 40, z_atr40_47.z47 => internal zone 47.
static void apply_zone_attributes_block(const char* blockKey, const JsonVariantConst& block) {
  if (!block.is<JsonObjectConst>()) return;
  const int8_t base = zone_block_base(blockKey);
  if (base < 0) return;
  const uint8_t minZoneNum = (uint8_t)base;
  const uint8_t maxZoneNum = (uint8_t)(base + 7);

  // Collect names for the block — written in a single atomic file pass below.
  char blockNames[8][ZONE_NAME_LEN];
  bool hasName[8];
  memset(blockNames, 0, sizeof(blockNames));
  memset(hasName,    0, sizeof(hasName));

  for (JsonPairConst kv : block.as<JsonObjectConst>()) {
    const char* key = kv.key().c_str();
    if (key[0] != 'z') continue;
    const int zoneNum = atoi(key + 1);  // "z40"->40, "z47"->47
    if (zoneNum < minZoneNum || zoneNum > maxZoneNum) continue;
    const uint8_t zone = (uint8_t)zoneNum;
    if (zone >= ZONE_COUNT) continue;

    const JsonObjectConst cfg = kv.value().as<JsonObjectConst>();
    if (cfg.isNull()) continue;

    const uint8_t rel = zone - (uint8_t)base;
    if (cfg["n"].is<const char*>()) {
      strlcpy(blockNames[rel], cfg["n"].as<const char*>(), ZONE_NAME_LEN);
      hasName[rel] = true;
    }
    if (cfg["by"].is<bool>()) gZoneManager.setBypassed(zone,    cfg["by"].as<bool>(),  false);
    if (cfg["ed"].is<bool>()) gZoneManager.setEntryDelay(zone,  cfg["ed"].as<bool>(),  false);
    if (cfg["xd"].is<bool>()) gZoneManager.setExitDelay(zone,   cfg["xd"].as<bool>(),  false);
    if (cfg["x24"].is<bool>()) gZoneManager.set24h(zone,        cfg["x24"].as<bool>(), false);
    if (cfg["pm"].is<bool>()) gZoneManager.setPerimeter(zone,   cfg["pm"].as<bool>(),  false);
    if (cfg["sl"].is<bool>()) gZoneManager.setSilent(zone,      cfg["sl"].as<bool>(),  false);
    if (cfg["rf"].is<bool>()) gZoneManager.setRF(zone,          cfg["rf"].as<bool>(),  false);
    if (cfg["ch"].is<bool>()) gZoneManager.setChime(zone,       cfg["ch"].as<bool>(),  false);
  }

  // Print what we are about to write
  Serial.printf("[ZONE] saving block %s (base=%u):\n", blockKey, (unsigned)base);
  for (uint8_t r = 0; r < 8; r++) {
    if (hasName[r]) Serial.printf("[ZONE]   Z%02u -> \"%s\"\n", (unsigned)(base + r), blockNames[r]);
  }

  // Single atomic write: attributes + all 48 names (block names overridden, rest preserved).
  const bool saveOk = gZoneManager.saveWithBlockNames((uint8_t)base, blockNames, hasName, 8);
  if (!saveOk) {
    Serial.printf("[ZONE] saveWithBlockNames FAILED for block %s\n", blockKey);
  }

  // Read back immediately to verify what landed on disk
  Serial.printf("[ZONE] readback block %s:\n", blockKey);
  char rbName[ZONE_NAME_LEN];
  for (uint8_t r = 0; r < 8; r++) {
    rbName[0] = '\0';
    gZoneManager.getName((uint8_t)(base + r), rbName, sizeof(rbName));
    Serial.printf("[ZONE]   Z%02u = \"%s\"%s\n",
                  (unsigned)(base + r), rbName,
                  (hasName[r] && strcmp(rbName, blockNames[r]) != 0) ? " *** MISMATCH ***" : "");
  }
}

// --- cfgIndex-driven attribute sync ---
// On connect: fetch shared attr key "cfgIndex" which contains {keyName:{ver,updatedTs},...}.
// Compare each key's server version vs local version stored in /cfgIndex.json.
// Fetch only data keys where server_ver > local_ver, one at a time.
// After applying each key: update /cfgIndex.json + publish client attr "cfgIndex".
// On attr/set push with "cfgIndex" key: immediately start selective key fetch.

struct CfgKeyMeta { uint32_t ver; uint32_t ts; };  // ts in seconds

static constexpr uint8_t  kAttrFetchKeyCount       = 8;
static constexpr uint32_t kAttrFetchTimeoutMs       = 6000;
static constexpr uint32_t kAttrFetchRetryIntervalMs = 30000;
static constexpr uint8_t  kAttrFetchMaxRetries      = 3;

static const struct {
  const char* keyName;   // data key name (attr/request + cfgIndex object key)
  const char* verField;  // cfgIndex.json field for version
  const char* tsField;   // cfgIndex.json field for timestamp (seconds)
} kCfgIdxMap[kAttrFetchKeyCount] = {
  {"config",     "cfg_config_ver",     "cfg_config_ts"},
  {"users",      "cfg_users_ver",      "cfg_users_ts"},
  {"z_atr00_07", "cfg_z_atr00_07_ver", "cfg_z_atr00_07_ts"},
  {"z_atr08_15", "cfg_z_atr08_15_ver", "cfg_z_atr08_15_ts"},
  {"z_atr16_23", "cfg_z_atr16_23_ver", "cfg_z_atr16_23_ts"},
  {"z_atr24_31", "cfg_z_atr24_31_ver", "cfg_z_atr24_31_ts"},
  {"z_atr32_39", "cfg_z_atr32_39_ver", "cfg_z_atr32_39_ts"},
  {"z_atr40_47", "cfg_z_atr40_47_ver", "cfg_z_atr40_47_ts"},
};

static CfgKeyMeta g_localCfgIdx[kAttrFetchKeyCount]  = {};  // loaded from /cfgIndex.json
static CfgKeyMeta g_serverCfgIdx[kAttrFetchKeyCount] = {};  // versions from server cfgIndex

enum eAttrFetchState {
  AFS_IDLE,
  AFS_FETCH_META,  // about to send cfgIndex request
  AFS_WAIT_META,   // waiting for cfgIndex response
  AFS_FETCH_KEY,   // about to send a data key request
  AFS_WAIT_KEY,    // waiting for a data key response
};

static uint8_t         g_pendingFetchMask = 0;   // bit N set = key N needs fetching
static uint8_t         g_attrFetchKeyIdx  = 0;   // current data key being fetched
static uint8_t         g_attrFetchReqId   = 0;
static uint8_t         g_attrFetchWaitId  = 0;
static uint32_t        g_attrFetchLastMs  = 0;
static uint32_t        g_attrRetryAfterMs = 0;
static uint8_t         g_attrFetchKeyRetries = 0;
static eAttrFetchState g_attrFetchState   = AFS_IDLE;

// ---- cfgIndex SPIFFS helpers ----

static void cfgIndex_save_to_spiffs() {
  // Build {"config":{"ver":N,"updatedTs":N},...} with snprintf — no stack-heavy JsonDocument.
  // ts stored internally in seconds; file stores milliseconds to match server format.
  char buf[512];
  int n = snprintf(buf, sizeof(buf), "{");
  for (uint8_t i = 0; i < kAttrFetchKeyCount; ++i) {
    if (i > 0) n += snprintf(buf + n, sizeof(buf) - n, ",");
    n += snprintf(buf + n, sizeof(buf) - n,
                  "\"%s\":{\"ver\":%lu,\"updatedTs\":%lu}",
                  kCfgIdxMap[i].keyName,
                  (unsigned long)g_localCfgIdx[i].ver,
                  (unsigned long)g_localCfgIdx[i].ts);
  }
  snprintf(buf + n, sizeof(buf) - n, "}");
  File f = SPIFFS.open("/cfgIndex.tmp", FILE_WRITE);
  if (!f) { Serial.println(F("cfgIndex: open tmp failed")); return; }
  f.print(buf);
  f.close();
  SPIFFS.remove("/cfgIndex.json");
  SPIFFS.rename("/cfgIndex.tmp", "/cfgIndex.json");
  Serial.println(F("cfgIndex: saved"));
}

static void cfgIndex_publish() {
  // Build {"cfgIndex":{"config":{"ver":N,"updatedTs":N},...}} manually.
  char buf[512];
  int n = snprintf(buf, sizeof(buf), "{\"cfgIndex\":{");
  for (uint8_t i = 0; i < kAttrFetchKeyCount; ++i) {
    if (i > 0) n += snprintf(buf + n, sizeof(buf) - n, ",");
    n += snprintf(buf + n, sizeof(buf) - n,
                  "\"%s\":{\"ver\":%lu,\"updatedTs\":%lu}",
                  kCfgIdxMap[i].keyName,
                  (unsigned long)g_localCfgIdx[i].ver,
                  (unsigned long)g_localCfgIdx[i].ts);
  }
  snprintf(buf + n, sizeof(buf) - n, "}}");
  publish_direct("attr/pub", buf, false);
}

static void cfgIndex_publish_invalid() {
  publish_direct("attr/pub", "{\"cfgIndex\":\"Invalid\"}", false);
}

// ---- process_server_cfg_index: compare versions, populate pending fetch mask ----

static void process_server_cfg_index(JsonObjectConst serverIndex) {
  if (serverIndex.isNull()) {
    Serial.println(F("cfgIndex: null/missing on server -> Invalid"));
    cfgIndex_publish_invalid();
    g_attrFetchState   = AFS_IDLE;
    g_attrRetryAfterMs = millis() + kAttrFetchRetryIntervalMs;
    return;
  }
  for (uint8_t i = 0; i < kAttrFetchKeyCount; ++i) {
    JsonObjectConst entry = serverIndex[kCfgIdxMap[i].keyName].as<JsonObjectConst>();
    if (entry.isNull()) continue;  // key not present on server, skip
    const uint32_t sVer = entry["ver"] | (uint32_t)0;
    if (sVer > g_localCfgIdx[i].ver) {
      g_serverCfgIdx[i].ver = sVer;
      g_serverCfgIdx[i].ts  = entry["updatedTs"] | (uint32_t)0;
      g_pendingFetchMask |= (1 << i);
    }
  }
  // Only update state if not already mid-fetch (new pending bits picked up by tick)
  if (g_attrFetchState != AFS_FETCH_KEY && g_attrFetchState != AFS_WAIT_KEY) {
    g_attrFetchState = (g_pendingFetchMask == 0) ? AFS_IDLE : AFS_FETCH_KEY;
    if (g_pendingFetchMask == 0) Serial.println(F("cfgIndex: all keys up to date"));
  }
}

// ---- handle_config_attr (version guard removed — cfgIndex is the gatekeeper) ----

static void handle_config_attr(JsonObjectConst cfg) {
  if (cfg.isNull()) return;

  // Alarm timers (applied immediately to alarm state machine)
  if (cfg["enDelay"].is<int>()) {
    systemConfig.entry_delay_time = (uint8_t)cfg["enDelay"].as<int>();
    myAlarm_pannel.set_entry_delay_timer_interval(systemConfig.entry_delay_time);
  }
  if (cfg["xtDelay"].is<int>()) {
    systemConfig.exit_delay_time = (uint8_t)cfg["xtDelay"].as<int>();
    myAlarm_pannel.set_exit_delay_timer_interval(systemConfig.exit_delay_time);
  }
  if (cfg["bellTout"].is<int>()) {
    systemConfig.bell_time_out = (uint16_t)cfg["bellTout"].as<int>();
    myAlarm_pannel.set_bell_time_timer_interval(systemConfig.bell_time_out);
  }
  if (cfg["beepTout"].is<int>()) systemConfig.beep_time_out = (uint16_t)cfg["beepTout"].as<int>();
  if (cfg["debTm"].is<int>())    systemConfig.sensor_debounce_time = (uint8_t)cfg["debTm"].as<int>();

  // Output enables
  if (cfg["bellEn"].is<bool>()) systemConfig.siren_en = cfg["bellEn"].as<bool>();
  if (cfg["beepEn"].is<bool>()) systemConfig.beep_en  = cfg["beepEn"].as<bool>();

  // GSM call settings
  if (cfg["callEn"].is<bool>())   systemConfig.call_en       = cfg["callEn"].as<bool>();
  if (cfg["callAtmpt"].is<int>()) systemConfig.call_attempts = (uint8_t)cfg["callAtmpt"].as<int>();
  if (cfg["lastSender"].is<const char*>())
    strlcpy(systemConfig.last_sms_sender, cfg["lastSender"].as<const char*>(), sizeof(systemConfig.last_sms_sender));

  // Installer credentials
  if (cfg["instNo"].is<const char*>())
    strlcpy(systemConfig.inst_no,  cfg["instNo"].as<const char*>(),  sizeof(systemConfig.inst_no));
  if (cfg["instPas"].is<const char*>())
    strlcpy(systemConfig.inst_pas, cfg["instPas"].as<const char*>(), sizeof(systemConfig.inst_pas));
  if (cfg["cliLevel"].is<int>())
    systemConfig.cli_access_level = (uint8_t)cfg["cliLevel"].as<int>();

  // WiFi credentials (effective after restart)
  if (cfg["wstaEn"].is<bool>())  systemConfig.wifi_sta_en = cfg["wstaEn"].as<bool>();
  if (cfg["wapEn"].is<bool>())   systemConfig.wifiap_en   = cfg["wapEn"].as<bool>();
  if (cfg["wssid"].is<const char*>())
    strlcpy(systemConfig.wifissid_sta, cfg["wssid"].as<const char*>(),   sizeof(systemConfig.wifissid_sta));
  if (cfg["wapssid"].is<const char*>())
    strlcpy(systemConfig.wifissid_ap,  cfg["wapssid"].as<const char*>(), sizeof(systemConfig.wifissid_ap));
  if (cfg["wstaPw"].is<const char*>())
    strlcpy(systemConfig.wifipass,     cfg["wstaPw"].as<const char*>(),  sizeof(systemConfig.wifipass));

  // MQTT credentials (effective after reconnect)
  if (cfg["mqttEn"].is<bool>())  systemConfig.mqtt_en = cfg["mqttEn"].as<bool>();
#ifndef MQTT_SECURE
  // Only update local broker credentials when not using hardcoded secure config
  if (cfg["mqttServer"].is<const char*>()) {
    strlcpy(systemConfig.mqtt_server, cfg["mqttServer"].as<const char*>(), sizeof(systemConfig.mqtt_server));
    strlcpy(mqttServer, systemConfig.mqtt_server, sizeof(mqttServer));
  }
  if (cfg["mqttPort"].is<int>()) {
    systemConfig.mqtt_port = (uint16_t)cfg["mqttPort"].as<int>();
    mqtt_port = systemConfig.mqtt_port;
  }
  if (cfg["mqttUser"].is<const char*>()) {
    strlcpy(systemConfig.mqtt_user, cfg["mqttUser"].as<const char*>(), sizeof(systemConfig.mqtt_user));
    strlcpy(mqtt_username, systemConfig.mqtt_user, sizeof(mqtt_username));
  }
  if (cfg["mqttPass"].is<const char*>()) {
    strlcpy(systemConfig.mqtt_pass, cfg["mqttPass"].as<const char*>(), sizeof(systemConfig.mqtt_pass));
    strlcpy(mqtt_password, systemConfig.mqtt_pass, sizeof(mqtt_password));
  }
#endif

  systemConfig.config_ver        = cfg["ver"]       | (uint32_t)0;
  systemConfig.config_updated_ts = cfg["updatedTs"] | (uint64_t)0;
  eeprom_save();
  Serial.printf("attr/config: applied ver=%lu\n", (unsigned long)systemConfig.config_ver);
}

static void apply_users_attr(JsonObjectConst usersIn);  // forward declaration

static void handle_users_attr(JsonVariantConst value) {
  if (value.isNull()) return;
  apply_users_attr(value.as<JsonObjectConst>());
}

// ---- apply_data_key_content: apply one data key + update cfgIndex ----

static void apply_data_key_content(uint8_t keyIdx, JsonVariantConst value) {
  const char* keyName = kCfgIdxMap[keyIdx].keyName;
  if (strcmp(keyName, "config") == 0) {
    handle_config_attr(value.as<JsonObjectConst>());
  } else if (strcmp(keyName, "users") == 0) {
    handle_users_attr(value);
  } else {
    apply_zone_attributes_block(keyName, value);  // single write: attrs + names
    gZoneManager.syncToEngine(zoneEngine);
  }
  // Persist updated version and publish client attr immediately
  g_localCfgIdx[keyIdx].ver = g_serverCfgIdx[keyIdx].ver;
  g_localCfgIdx[keyIdx].ts  = g_serverCfgIdx[keyIdx].ts;
  cfgIndex_save_to_spiffs();
  cfgIndex_publish();
  Serial.printf("attr: applied key=%s ver=%lu\n",
                keyName, (unsigned long)g_localCfgIdx[keyIdx].ver);
}

// ---- attr/res response handler ----

static void handle_attr_response(const char* payload) {
  if (g_attrFetchState != AFS_WAIT_META && g_attrFetchState != AFS_WAIT_KEY) {
    Serial.println(F("attr/res: not waiting, ignore"));
    return;
  }
  DynamicJsonDocument doc(2048);
  if (deserializeJson(doc, payload) != DeserializationError::Ok) {
    Serial.println(F("attr/res: parse failed"));
    if (g_attrFetchState == AFS_WAIT_META) {
      cfgIndex_publish_invalid();
      g_attrFetchState   = AFS_IDLE;
      g_attrRetryAfterMs = millis() + kAttrFetchRetryIntervalMs;
    } else {
      g_pendingFetchMask &= ~(1 << g_attrFetchKeyIdx);
      g_attrFetchState = AFS_FETCH_KEY;
    }
    return;
  }
  if (!doc["id"].is<int>()) { Serial.println(F("attr/res: no id, ignore")); return; }
  const uint8_t respId = (uint8_t)doc["id"].as<int>();
  if (respId != g_attrFetchWaitId) {
    Serial.printf("attr/res: id mismatch resp=%u wait=%u\n", respId, g_attrFetchWaitId);
    return;
  }
  JsonVariantConst value = doc["value"];
  if (value.isNull()) {
    Serial.println(F("attr/res: no value field"));
    if (g_attrFetchState == AFS_WAIT_META) {
      cfgIndex_publish_invalid();
      g_attrFetchState   = AFS_IDLE;
      g_attrRetryAfterMs = millis() + kAttrFetchRetryIntervalMs;
    } else {
      g_pendingFetchMask &= ~(1 << g_attrFetchKeyIdx);
      g_attrFetchState = AFS_FETCH_KEY;
    }
    return;
  }
  if (g_attrFetchState == AFS_WAIT_META) {
    process_server_cfg_index(value.as<JsonObjectConst>());
  } else {
    apply_data_key_content(g_attrFetchKeyIdx, value);
    g_pendingFetchMask &= ~(1 << g_attrFetchKeyIdx);
    g_attrFetchState = AFS_FETCH_KEY;
  }
}

static void mqtt_attr_fetch_start() {
  g_pendingFetchMask = 0;
  g_attrFetchState   = AFS_FETCH_META;
  g_attrRetryAfterMs = 0;
}

static void tick_attr_fetch_sm() {
  if (!client.connected()) return;

  switch (g_attrFetchState) {

    case AFS_IDLE:
      if (g_attrRetryAfterMs != 0 && millis() >= g_attrRetryAfterMs) {
        g_attrRetryAfterMs = 0;
        g_attrFetchState = AFS_FETCH_META;
      }
      return;

    case AFS_FETCH_META: {
      g_attrFetchWaitId = ++g_attrFetchReqId;
      StaticJsonDocument<80> req;
      req["id"]     = g_attrFetchWaitId;
      req["client"] = false;
      req["key"]    = "cfgIndex";
      char reqBuf[96];
      serializeJson(req, reqBuf, sizeof(reqBuf));
      publish_direct("attr/request", reqBuf, false);
      g_attrFetchLastMs = millis();
      g_attrFetchState  = AFS_WAIT_META;
      Serial.printf("attr fetch: requesting cfgIndex (id=%u)\n", g_attrFetchWaitId);
      return;
    }

    case AFS_WAIT_META:
      if ((millis() - g_attrFetchLastMs) > kAttrFetchTimeoutMs) {
        Serial.println(F("attr fetch: cfgIndex timeout -> Invalid"));
        cfgIndex_publish_invalid();
        g_attrFetchState   = AFS_IDLE;
        g_attrRetryAfterMs = millis() + kAttrFetchRetryIntervalMs;
      }
      return;

    case AFS_FETCH_KEY: {
      if (g_pendingFetchMask == 0) {
        Serial.println(F("attr fetch: all pending keys done"));
        g_attrFetchState = AFS_IDLE;
        return;
      }
      uint8_t idx = 0;
      while (idx < kAttrFetchKeyCount && !(g_pendingFetchMask & (1 << idx))) ++idx;
      if (idx >= kAttrFetchKeyCount) { g_attrFetchState = AFS_IDLE; return; }
      g_attrFetchKeyIdx     = idx;
      g_attrFetchKeyRetries = 0;
      g_attrFetchWaitId = ++g_attrFetchReqId;
      StaticJsonDocument<80> req;
      req["id"]     = g_attrFetchWaitId;
      req["client"] = false;
      req["key"]    = kCfgIdxMap[idx].keyName;
      char reqBuf[96];
      serializeJson(req, reqBuf, sizeof(reqBuf));
      publish_direct("attr/request", reqBuf, false);
      g_attrFetchLastMs = millis();
      g_attrFetchState  = AFS_WAIT_KEY;
      Serial.printf("attr fetch: requesting key=%s (id=%u)\n", kCfgIdxMap[idx].keyName, g_attrFetchWaitId);
      return;
    }

    case AFS_WAIT_KEY:
      if ((millis() - g_attrFetchLastMs) > kAttrFetchTimeoutMs) {
        if (g_attrFetchKeyRetries < kAttrFetchMaxRetries) {
          ++g_attrFetchKeyRetries;
          Serial.printf("attr fetch: timeout key=%s retry %u/%u\n",
                        kCfgIdxMap[g_attrFetchKeyIdx].keyName,
                        g_attrFetchKeyRetries, kAttrFetchMaxRetries);
        } else {
          Serial.printf("attr fetch: timeout key=%s max retries, skip\n",
                        kCfgIdxMap[g_attrFetchKeyIdx].keyName);
          g_pendingFetchMask &= ~(1 << g_attrFetchKeyIdx);
        }
        g_attrFetchState = AFS_FETCH_KEY;
      }
      return;
  }
}

// Converts incoming MQTT users object (u0–u7) to users.json flat array.
// Keys are identical on both sides: en, nm, tp, smsEn, callEn.
// Remote IDs are stored separately in remotes.bin (RemoteStorage), indexed by slot = user index.
static void apply_users_attr(JsonObjectConst usersIn) {
  DynamicJsonDocument doc(JSON_DOC_SIZE_USER_DATA);
  JsonArray users = doc.to<JsonArray>();

  // Pre-fill 8 empty slots so the array always has 8 entries
  for (uint8_t i = 0; i < 8; ++i) {
    JsonObject u = users.createNestedObject();
    u["en"]     = false;
    u["nm"]     = "";
    u["tp"]     = "";
    u["smsEn"]  = false;
    u["callEn"] = false;
  }

  for (JsonPairConst kv : usersIn) {
    const char* key = kv.key().c_str();
    if (key[0] != 'u') continue;
    const int idx = atoi(key + 1);  // "u0"->0, "u7"->7
    if (idx < 0 || idx >= 8) continue;
    JsonObjectConst src = kv.value().as<JsonObjectConst>();
    if (src.isNull()) continue;
    JsonObject u = users[idx].as<JsonObject>();
    u["en"]     = src["en"]     | false;
    u["nm"]     = src["nm"]     | "";
    u["tp"]     = src["tp"]     | "";
    u["smsEn"]  = src["smsEn"]  | false;
    u["callEn"] = src["callEn"] | false;
  }

  // Atomic write via tmp file
  File wf = SPIFFS.open("/users.tmp", FILE_WRITE);
  if (!wf) { Serial.println(F("attr/users: open tmp failed")); return; }
  serializeJson(doc, wf);
  wf.close();
  SPIFFS.remove("/users.json");
  if (SPIFFS.rename("/users.tmp", "/users.json")) {
    Serial.println(F("attr/users: saved"));
  } else {
    Serial.println(F("attr/users: rename failed"));
  }
}

static void apply_attribute_updates(JsonObjectConst data) {
  if (data.isNull()) return;

  // cfgIndex version push — triggers selective attr re-fetch
  JsonVariantConst cfgIdxVal = data["cfgIndex"];
  if (!cfgIdxVal.isNull()) {
    process_server_cfg_index(cfgIdxVal.as<JsonObjectConst>());
    return;
  }

  // Direct zone attr block push — apply immediately (same path as attr/res)
  for (JsonPairConst kv : data) {
    const char* key = kv.key().c_str();
    if (zone_block_base(key) >= 0) {
      Serial.printf("[ZONE] attr/set direct push: %s\n", key);
      apply_zone_attributes_block(key, kv.value());
      gZoneManager.syncToEngine(zoneEngine);
    }
  }
}

static void handle_attr_message(const char* payload) {
  const unsigned payloadLen = (unsigned)strlen(payload);
  Serial.printf("[DBG] handle_attr_message entered len=%u heap=%u\n",
                payloadLen, (unsigned)ESP.getFreeHeap());

  // Zone attr block payloads are ~860 bytes; ArduinoJson needs ~3x for internal overhead.
  DynamicJsonDocument doc(3072);
  Serial.println(F("[DBG] doc allocated"));

  const DeserializationError err = deserializeJson(doc, payload);
  Serial.printf("[DBG] deserializeJson result: %s\n", err.c_str());
  if (err) {
    Serial.printf("MQTT attr parse failed: %s (len=%u)\n", err.c_str(), payloadLen);
    return;
  }

  // Force-push format: {"device":"...","data":{"cfgIndex":{...}}} or {"data":{"z_atr00_07":{...}}}
  JsonObjectConst data = doc["data"].as<JsonObjectConst>();
  if (data.isNull()) {
    Serial.println(F("MQTT attr: no data field"));
    return;
  }
  Serial.println(F("[DBG] calling apply_attribute_updates"));
  apply_attribute_updates(data);
}

static void handle_rpc_sms(const char* reqId, JsonObjectConst params) {
  const char* msg = params["msg"] | "";
  const char* target = params["tp"] | "";
  if (msg[0] == '\0' || target[0] == '\0') {
    mqtt_publish_rpc_failure(reqId, "INVALID_PARAM", "msg and tp are required");
    return;
  }

  creatSMS(msg, 4, target);
  mqtt_publish_rpc_success(reqId, "status", "queued");
}

static void handle_rpc_state_change(const char* reqId, JsonObjectConst params) {
  const char* state = params["state"] | "";
  const uint8_t userId = (uint8_t)atoi(params["userId"] | "0");

  if (strcmp(state, "arm") == 0) {
    myAlarm_pannel.set_arm_mode(AS_ITIS_NO_BYPASS);
    myAlarm_pannel.set_system_state(SYS1_IDEAL, APP, userId);
    mqtt_publish_rpc_success(reqId, "state", "arm");
    return;
  }
  if (strcmp(state, "away") == 0) {
    myAlarm_pannel.set_arm_mode(AS_ITIS_BYPASS);
    myAlarm_pannel.set_system_state(SYS1_IDEAL, APP, userId);
    mqtt_publish_rpc_success(reqId, "state", "away");
    return;
  }
  if (strcmp(state, "disarm") == 0) {
    myAlarm_pannel.set_system_state(DEACTIVE, APP, userId);
    eCurrent_state = DEACTIVE;
    mqtt_publish_rpc_success(reqId, "state", "disarm");
    return;
  }

  mqtt_publish_rpc_failure(reqId, "INVALID_PARAM", "unsupported state");
}

static void handle_rpc_siren(const char* reqId, JsonObjectConst params) {
  const char* state = params["state"] | "";
  if (strcmp(state, "on") == 0) {
    xEventGroupSetBits(EventRTOS_siren, TASK_3_BIT);
    mqtt_publish_rpc_success(reqId, "state", "on");
    return;
  }
  if (strcmp(state, "off") == 0 || strcmp(state, "mute") == 0) {
    xEventGroupSetBits(EventRTOS_siren, TASK_1_BIT);
    mqtt_publish_rpc_success(reqId, "state", "off");
    return;
  }

  mqtt_publish_rpc_failure(reqId, "INVALID_PARAM", "unsupported siren state");
}

static void handle_rpc_conf(const char* reqId, JsonObjectConst params) {
  const char* state = params["state"] | "";
  if (strcmp(state, "reload") != 0) {
    mqtt_publish_rpc_failure(reqId, "INVALID_PARAM", "unsupported conf state");
    return;
  }

  eeprom_load(0);
  mqtt_publish_latest_attributes();
  mqtt_publish_telemetry();
  mqtt_publish_rpc_success(reqId, "state", "reloaded");
}

static void handle_rpc_power(const char* reqId, JsonObjectConst params) {
  const char* state = params["state"] | "";
  if (strcmp(state, "reboot") != 0) {
    mqtt_publish_rpc_failure(reqId, "INVALID_PARAM", "unsupported power state");
    return;
  }

  mqtt_publish_status("rebooting", "rpc_reboot", true);
  mqtt_publish_rpc_success(reqId, "state", "rebooting");
  drain_mqtt_tx_queue();
  client.loop();
  delay(150);
  ESP.restart();
}

static void handle_rpc_message(const char* payload) {
  StaticJsonDocument<768> doc;
  const DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("MQTT rpc parse failed: %s\n", err.c_str());
    return;
  }

  const JsonObjectConst rpc = doc["data"].is<JsonObject>() ? doc["data"].as<JsonObjectConst>() : doc.as<JsonObjectConst>();
  const char* method = rpc["method"] | "";
  const JsonObjectConst params = rpc["params"].as<JsonObjectConst>();

  char reqId[24] = {0};
  if (rpc["reqId"].is<const char*>()) {
    strlcpy(reqId, rpc["reqId"].as<const char*>(), sizeof(reqId));
  } else if (rpc["id"].is<int>()) {
    snprintf(reqId, sizeof(reqId), "%d", rpc["id"].as<int>());
  } else if (doc["reqId"].is<const char*>()) {
    strlcpy(reqId, doc["reqId"].as<const char*>(), sizeof(reqId));
  }

  if (method[0] == '\0') {
    mqtt_publish_rpc_failure(reqId, "INVALID_REQUEST", "method is required");
    return;
  }

  if (strcmp(method, "sms") == 0) {
    handle_rpc_sms(reqId, params);
  } else if (strcmp(method, "stateChange") == 0) {
    handle_rpc_state_change(reqId, params);
  } else if (strcmp(method, "siren") == 0) {
    handle_rpc_siren(reqId, params);
  } else if (strcmp(method, "conf") == 0) {
    handle_rpc_conf(reqId, params);
  } else if (strcmp(method, "power") == 0) {
    handle_rpc_power(reqId, params);
  } else {
    mqtt_publish_rpc_failure(reqId, "UNKNOWN_METHOD", "unsupported rpc method");
  }
}

static void drain_mqtt_inbound_queue() {
  if (xQueue_mqtt_inbound == nullptr) return;

  MqttInboundMsg msg {};
  while (xQueueReceive(xQueue_mqtt_inbound, &msg, 0) == pdPASS) {
    if (strstr(msg.topic, "/attr/set") != nullptr) {
      handle_attr_message(msg.payload);
#ifdef _DEBUG
      Serial.println(F("MQTT RX handled as attr update"));
#endif
    } else if (strstr(msg.topic, "/attr/res") != nullptr) {
      handle_attr_response(msg.payload);
#ifdef _DEBUG
      Serial.println(F("MQTT RX handled as attr response"));
#endif
    } else if (strstr(msg.topic, "/rpc/req") != nullptr) {
      handle_rpc_message(msg.payload);
#ifdef _DEBUG
      Serial.println(F("MQTT RX handled as rpc req"));
#endif
    } else {
#ifdef _DEBUG
      Serial.printf("MQTT RX unhandled topic: %s\n", msg.topic);
#endif
    }
  }
}

}  // namespace

// Load /cfgIndex.json from SPIFFS into g_localCfgIdx.
// Call during boot (configLoad mode 0) after SPIFFS is mounted.
// If file is missing or corrupt, all versions default to 0 (full sync on next connect).
void mqtt_load_cfg_index() {
  File f = SPIFFS.open("/cfgIndex.json");
  if (!f) {
    Serial.println(F("cfgIndex.json missing — all versions reset to 0 (full sync on connect)"));
    memset(g_localCfgIdx, 0, sizeof(g_localCfgIdx));
    return;
  }
  StaticJsonDocument<640> doc;
  const DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.printf("cfgIndex.json parse error: %s — versions reset to 0\n", err.c_str());
    memset(g_localCfgIdx, 0, sizeof(g_localCfgIdx));
    return;
  }
  for (uint8_t i = 0; i < kAttrFetchKeyCount; ++i) {
    JsonObjectConst entry = doc[kCfgIdxMap[i].keyName].as<JsonObjectConst>();
    if (entry.isNull()) continue;
    g_localCfgIdx[i].ver = entry["ver"]       | (uint32_t)0;
    g_localCfgIdx[i].ts  = entry["updatedTs"] | (uint32_t)0;
  }
  Serial.println(F("cfgIndex.json loaded"));
}

void mqtt_invalidate_zone_cfg_versions() {
  // kCfgIdxMap indices 2–7 are zone attr keys (z_atr00_07 … z_atr40_47).
  // Reset their local versions to 0 so MQTT will re-fetch them on next connect.
  bool changed = false;
  for (uint8_t i = 2; i < kAttrFetchKeyCount; ++i) {
    if (g_localCfgIdx[i].ver != 0 || g_localCfgIdx[i].ts != 0) {
      g_localCfgIdx[i].ver = 0;
      g_localCfgIdx[i].ts  = 0;
      changed = true;
    }
  }
  if (changed) {
    cfgIndex_save_to_spiffs();
    Serial.println(F("[ZONE] zones.bin was reinitialised — zone attr cfgIndex versions cleared, will re-fetch on MQTT connect"));
  }
}

bool mqtt_enable = false;
xQueueHandle xQueue_mqtt_Qhdlr = nullptr;
QueueHandle_t xQueue_mqtt_tx = nullptr;
PubSubClient client(espClient);

void callback_onMQTT_connection(_callbackFunctionType7 pFn) { fn_onMQTT_connection = pFn; }
void callback_onMQTT_disconnection(_callbackFunctionType7 pFn) { fn_onMQTT_disconnection = pFn; }

void mqtt_rx_init() {
  if (xQueue_mqtt_Qhdlr == nullptr) {
    xQueue_mqtt_Qhdlr = xQueueCreate(10, sizeof(DataBuffer));
    if (!xQueue_mqtt_Qhdlr) {
      Serial.println(F("MQTT RX queue create failed"));
    }
  }

  if (xQueue_mqtt_inbound == nullptr) {
    xQueue_mqtt_inbound = xQueueCreate(kInboundQueueLen, sizeof(MqttInboundMsg));
    if (!xQueue_mqtt_inbound) {
      Serial.println(F("MQTT inbound queue create failed"));
    }
  }
}

void mqtt_tx_init() {
  if (xQueue_mqtt_tx != nullptr) return;
  xQueue_mqtt_tx = xQueueCreate(16, sizeof(MqttTxMsg));
  if (!xQueue_mqtt_tx) {
    Serial.println(F("MQTT TX queue create failed"));
  }
}

bool mqtt_is_connected() {
  return client.connected();
}

const char* mqtt_device_id() {
  return g_deviceId;
}

void setup_mqtt() {
#ifdef DEV_MAC
  strlcpy(g_deviceId, DEV_MAC, sizeof(g_deviceId));
  Serial.printf("[MQTT] DEV_MAC override: %s\n", g_deviceId);
#else
  WiFi.macAddress(mac);
  snprintf(g_deviceId, sizeof(g_deviceId), "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
#endif

  if (!mqtt_enable) {
    Serial.println(F("MQTT DISABLED"));
    return;
  }

#ifdef MQTT_SECURE
  espClient.setCACert(ca_cert);
  strlcpy(mqttServer, MQTT_SECURE_SERVER, sizeof(mqttServer));
  strlcpy(mqtt_username, MQTT_SECURE_USERNAME, sizeof(mqtt_username));
  strlcpy(mqtt_password, MQTT_SECURE_PASSWORD, sizeof(mqtt_password));
  mqtt_port = MQTT_SECURE_PORT;
#else
  // Local broker — credentials loaded at boot into systemConfig from config.json
  strlcpy(mqttServer,    systemConfig.mqtt_server, sizeof(mqttServer));
  strlcpy(mqtt_username, systemConfig.mqtt_user,   sizeof(mqtt_username));
  strlcpy(mqtt_password, systemConfig.mqtt_pass,   sizeof(mqtt_password));
  mqtt_port = systemConfig.mqtt_port;
#endif

  // PubSubClient packet buffer is set at compile time via MQTT_MAX_PACKET_SIZE in platformio.ini
  client.setKeepAlive(60);
  client.setServer(mqttServer, mqtt_port);
  client.setCallback(callback);
}

void reconnectMQTT() {
  if (!mqtt_enable) return;
  if (WiFi.status() != WL_CONNECTED) {
    if (fn_onMQTT_disconnection) fn_onMQTT_disconnection();
    return;
  }

#ifdef MQTT_SECURE
  if (!tls_time_ready()) {
    request_wifi_time_sync_if_needed();
    Serial.println(F("TLS time not ready, waiting for NTP"));
    log_tls_time_state();
    return;
  }
  g_lastNtpRequestMs = 0;
  espClient.setCACert(ca_cert);
#endif

  if (client.connected()) return;

  char willPayload[96];
  build_status_payload(willPayload, sizeof(willPayload), "offline", nullptr);

  char willTopic[kTopicBufferSize];
  build_topic(willTopic, sizeof(willTopic), "stat");

  // Rate-limit reconnect attempts — only try once every 5 seconds
  static uint32_t lastReconnectAttemptMs = 0;
  if (millis() - lastReconnectAttemptMs < 5000) return;
  lastReconnectAttemptMs = millis();

  char clientId[64];
  snprintf(clientId, sizeof(clientId), "blackwire-fw-%s", g_deviceId);

  log_mqtt_connect_diagnostics();
  Serial.printf("Attempting MQTT connection to %s:%d...\n", mqttServer, mqtt_port);

  if (client.connect(clientId, mqtt_username, mqtt_password, willTopic, 1, true, willPayload)) {
#ifdef _DEBUG
    Serial.printf("MQTT connected. Device ID: %s\n", g_deviceId);
#endif
    setup_subscriptions();
    mqtt_publish_status("online", nullptr, true);
    mqtt_attr_fetch_start();
    drain_mqtt_tx_queue();
    if (fn_onMQTT_connection) fn_onMQTT_connection();
  } else {
    Serial.printf("MQTT connect failed, rc=%d — will retry in 5s\n", client.state());
    log_mqtt_connect_diagnostics();
    if (WiFi.status() != WL_CONNECTED) {
      if (fn_onMQTT_disconnection) fn_onMQTT_disconnection();
    }
  }
}

void mqtt_com_loop() {
  if (!mqtt_enable) return;
  if (!client.connected()) {
    reconnectMQTT();
  }

  drain_mqtt_inbound_queue();
  tick_attr_fetch_sm();
  drain_mqtt_tx_queue();
  client.loop();
}

void setup_subscriptions() {
  if (!mqtt_enable || !client.connected()) return;

  char topic[kTopicBufferSize];
  build_topic(topic, sizeof(topic), "attr/set");
  client.subscribe(topic);
  build_topic(topic, sizeof(topic), "attr/res");
  client.subscribe(topic);
  build_topic(topic, sizeof(topic), "rpc/req");
  client.subscribe(topic);
}

void callback(char *topic, byte *payload, unsigned int length) {
  char payloadBuffer[MQTT_RX_PAYLOAD_MAX];
  if (length >= sizeof(payloadBuffer)) {
    Serial.printf("MQTT RX: payload truncated (%u -> %u bytes) on topic %s\n",
                  length, (unsigned)(sizeof(payloadBuffer) - 1), topic);
    length = sizeof(payloadBuffer) - 1;
  }
  memcpy(payloadBuffer, payload, length);
  payloadBuffer[length] = '\0';
#ifdef _DEBUG
  Serial.printf_P(PSTR("MQTT RX %s => %s\n"), topic, payloadBuffer);
#endif
  mqtt_inbound_enqueue(topic, payloadBuffer);
}

void mqtt_publish_telemetry() {
  StaticJsonDocument<512> doc;
  char sens[25] = {0};
  build_sensor_pack(sens, sizeof(sens));

  add_timestamp_metadata(doc);
  char ipbuf[16];
  WiFi.localIP().toString().toCharArray(ipbuf, sizeof(ipbuf));
  doc["ip"] = ipbuf;
  doc["wrssi"] = WiFi.RSSI();
  doc["grssi"] = (int)getSignal_strength();
  doc["mode"] = arm_mode_to_str(myAlarm_pannel.get_arm_mode());
  doc["upTime"] = millis() / 1000UL;
  doc["gsm"] = !gsm_sim_ok ? "noSIM" : (!gsm_net_ok ? "noNet" : "ok");
  doc["vac"] = systemConfig.ac_power ? "ok" : "no";
  doc["vbat"] = 12.3f;
  doc["bCharg"] = systemConfig.battery_charging_en ? "on" : "off";
  doc["machineState"] = armed_state_to_str();
  doc["Home Armed"] = (myAlarm_pannel.get_system_state() != DEACTIVE);
  doc["alarmState"] = alarm_state_to_str();
  doc["siren"] = siren_is_active();
  doc["batteryLow"] = false;
  doc["trouble"] = compute_trouble();
  doc["sens"] = sens;

  mqtt_publish_json("tele", doc, false);
}

void mqtt_publish_latest_attributes() {
  StaticJsonDocument<384> doc;
  char sens[25] = {0};
  build_sensor_pack(sens, sizeof(sens));

  doc["fw_ver"] = MQTT_FIRMWARE_VERSION;
  doc["hw_ver"] = MQTT_HARDWARE_VERSION;
  doc["mode"] = arm_mode_to_str(myAlarm_pannel.get_arm_mode());
  doc["state"] = armed_state_to_str();
  doc["Home Armed"] = (myAlarm_pannel.get_system_state() != DEACTIVE);
  doc["alarmState"] = alarm_state_to_str();
  doc["siren"] = siren_is_active();
  doc["batteryLow"] = false;
  doc["trouble"] = compute_trouble();
  doc["sens"] = sens;

  mqtt_publish_json("attr/pub", doc, true);
}

void mqtt_publish_status(const char* status, const char* reason, bool retained) {
  char payload[96];
  build_status_payload(payload, sizeof(payload), status, reason);
  mqtt_tx_enqueue("stat", payload, retained);
}

void mqtt_publish_zone_event(uint8_t zone, bool isOpen) {
  StaticJsonDocument<256> doc;
  char sens[25] = {0};
  build_sensor_pack(sens, sizeof(sens));

  add_timestamp_metadata(doc);
  doc["event"] = "zone_state_changed";
  JsonObject data = doc.createNestedObject("data");
  data["zone"] = zone;
  data["state"] = isOpen ? "open" : "closed";
  doc["sens"] = sens;

  mqtt_publish_json("event", doc, false);
}

void mqtt_publish_alarm_event(const char* eventName, int zone, const char* reason, const char* alarmType) {
  StaticJsonDocument<256> doc;
  char sens[25] = {0};
  build_sensor_pack(sens, sizeof(sens));

  add_timestamp_metadata(doc);
  doc["event"] = eventName;
  JsonObject data = doc.createNestedObject("data");
  if (zone >= 0) data["zone"] = zone;
  if (reason != nullptr && reason[0] != '\0') data["reason"] = reason;
  if (alarmType != nullptr && alarmType[0] != '\0') data["alarmType"] = alarmType;
  doc["sens"] = sens;

  mqtt_publish_json("event", doc, false);
}

void mqtt_publish_state_action_event(const char* eventName, uint8_t userId, eInvoking_source source) {
  StaticJsonDocument<224> doc;
  char sens[25] = {0};
  char sourceBuf[16] = {0};
  build_sensor_pack(sens, sizeof(sens));
  get_eInvoker_type_to_char(source, sourceBuf);

  add_timestamp_metadata(doc);
  doc["event"] = eventName;
  JsonObject data = doc.createNestedObject("data");
  data["userId"] = userId;
  data["source"] = sourceBuf;
  doc["sens"] = sens;

  mqtt_publish_json("event", doc, false);
}

void mqtt_publish_power_event(bool powerOk) {
  mqtt_publish_alarm_event(powerOk ? "power_restore" : "power_fail", -1, powerOk ? "vac_ok" : "vac_fail", nullptr);
}

void mqtt_publish_sms_received_event(const char* message, const char* number) {
  StaticJsonDocument<256> doc;
  char sens[25] = {0};
  build_sensor_pack(sens, sizeof(sens));

  add_timestamp_metadata(doc);
  doc["event"] = "sms_received";
  JsonObject data = doc.createNestedObject("data");
  data["number"] = number;
  data["msg"] = message;
  doc["sens"] = sens;

  mqtt_publish_json("event", doc, false);
}

void mqtt_publish_boot_snapshot() {
  mqtt_publish_status("boot", "power_on", false);
  mqtt_publish_latest_attributes();
  mqtt_publish_telemetry();
}

void publish_incomming_sms_to_mqtt(char* local_smsbuffer, char* n) {
  mqtt_publish_sms_received_event(local_smsbuffer, n);
}
