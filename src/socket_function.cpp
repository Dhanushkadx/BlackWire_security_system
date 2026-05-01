// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------
#include "socket_function.h"
#include "mapping/zone_map.h"

// If you keep zone names in a JSON file, set this to 1
#define USE_ZONE_NAMES_JSON  1

AsyncWebSocket ws("/ws");

static char* ws_text_buf = nullptr;
static uint32_t ws_client_id = 0;
static size_t ws_expected_len = 0;
static constexpr size_t WS_TEXT_MAX_LEN = 16000;

static void wsReleaseTextBuffer() {
  if (ws_text_buf != nullptr) {
    free(ws_text_buf);
    ws_text_buf = nullptr;
  }
  ws_expected_len = 0;
  ws_client_id = 0;
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
		wsReleaseTextBuffer();
		Serial.printf("WebSocket client #%u disconnected\n", client->id());
		break;
		case WS_EVT_PONG:
		case WS_EVT_ERROR:
		break;
		case WS_EVT_DATA:
  		handleWebSocketMessage(client, arg, data, len);
  		break;
	}
}

static void wsSendErr(AsyncWebSocketClient* c, const char* page, const char* msg){
  StaticJsonDocument<192> r;
  r["respHeader"] = "err";
  r["page"] = page ? page : "";
  r["message"] = msg ? msg : "error";
  char buf[192];
  serializeJson(r, buf, sizeof(buf));
  c->text(buf);
}

static void wsSendOk(AsyncWebSocketClient* c, const char* page, const char* msg){
  StaticJsonDocument<192> r;
  r["respHeader"] = "ok";
  r["page"] = page ? page : "";
  r["message"] = msg;
  char buf[192];
  serializeJson(r, buf, sizeof(buf));
  c->text(buf);
}

// ---- send pages (modify your existing notifyClients_* to accept client*) ----
void sendPageSys(AsyncWebSocketClient* c){
  File f = SPIFFS.open("/config.json", FILE_READ);
  if(!f) { wsSendErr(c, "sys", "config.json open failed"); return; }

  DynamicJsonDocument d(JSON_DOC_SIZE_CONFIG_DATA);
  DeserializationError err = deserializeJson(d, f);
  f.close();
  if(err) { wsSendErr(c, "sys", "config.json parse failed"); return; }

#ifdef MQTT_SECURE
  d["mqttSecure"] = true;
#endif

  // Avoid deep-copy into a second doc (would silently truncate keys if dest is too small).: manually wrap the serialized config with the header.
  // {"respHeader":"sys","sysconf": ... }
  const char* prefix = "{\"respHeader\":\"sys\",\"sysconf\":";
  size_t prefLen = strlen(prefix);
  size_t cfgLen  = measureJson(d);
  size_t total   = prefLen + cfgLen + 1; // +1 for closing '}'

  char* out = (char*)malloc(total + 1); // +1 for null terminator
  if (!out) { wsSendErr(c, "sys", "out of memory"); return; }

  memcpy(out, prefix, prefLen);
  serializeJson(d, out + prefLen, cfgLen + 1);
  out[total - 1] = '}';
  out[total]     = '\0';
  Serial.print(F("[sendPageSys] total=")); Serial.print(total);
  Serial.print(F(" out=")); Serial.println(out);
  c->text(out);
  free(out);
}

void sendPageZones(AsyncWebSocketClient* c){
// Load zones.bin (or auto-create defaults if missing/corrupt)
if (!ZoneStorage::loadOrInit(SPIFFS, "/zones.bin", any_sensor_array, ZONE_COUNT)) {
    Serial.println(F("ZoneStorage: loadOrInit failed"));
} else {
    Serial.println(F("ZoneStorage: zones loaded/initialized OK"));
#ifdef _DEBUG
    ZoneStorage::printZones(SPIFFS, "/zones.bin", any_sensor_array, ZONE_COUNT);
#endif
}
  DynamicJsonDocument r(12288);
  r["respHeader"] = "zones";
  JsonArray arr = r.createNestedArray("zones");

  for(uint8_t i=0;i<ZONE_COUNT;i++){
    JsonObject z = arr.createNestedObject();
    // n/by/ed/xd/rf/x24/sl
    char name[ZONE_NAME_LEN];
	if(ZoneStorage::getName(SPIFFS, "/zones.bin", i, name, sizeof(name), ZONE_COUNT))
	z["n"] = name;
	else
	z["n"] = "";
    z["by"]  = (any_sensor_array[i].device_state & (1 << BIT_MASK_BYPASSED)) != 0;
    z["ed"]  = (any_sensor_array[i].device_state & (1 << BIT_MASK_ENTRY_DELAY)) != 0;
    z["xd"]  = (any_sensor_array[i].device_state & (1 << BIT_MASK_EXIT_DELAY)) != 0;
    z["rf"]  = (any_sensor_array[i].device_type  & (1 << BIT_MASK_RF)) != 0;
    z["x24"] = (any_sensor_array[i].device_type  & (1 << BIT_MASK_24H)) != 0;
    z["sl"]  = (any_sensor_array[i].device_type  & (1 << BIT_MASK_SILENT)) != 0;
    z["pm"]  = (any_sensor_array[i].device_type  & (1 << BIT_MASK_PERIMETER)) != 0;
    z["ch"]  = (any_sensor_array[i].device_type  & (1 << BIT_MASK_CHIME)) != 0;
    // 3 = unavailable (no hardware on this slot)
    z["status"] = isZoneActive(i) ? (uint8_t)zoneEngine.getState(i) : (uint8_t)3;
  }

  String out; serializeJson(r, out);
  c->text(out);
}

void sendPageContacts(AsyncWebSocketClient* c){
  DynamicJsonDocument r(JSON_DOC_SIZE_USER_DATA + 64);
  r["respHeader"] = "contacts";
  JsonArray users = r.createNestedArray("users");

  // Pre-fill 8 empty slots so frontend always gets a full array
  for (uint8_t i = 0; i < 8; i++) {
    JsonObject u = users.createNestedObject();
    u["nm"]     = "";
    u["tp"]     = "";
    u["smsEn"]  = false;
    u["callEn"] = false;
    u["en"]     = false;
  }

  // Load users.json — root is a flat array [{en,nm,tp,smsEn,callEn}, ...]
  File f = SPIFFS.open("/users.json", FILE_READ);
  if (f) {
    DynamicJsonDocument d(JSON_DOC_SIZE_USER_DATA);
    auto err = deserializeJson(d, f);
    f.close();

    if (!err && d.is<JsonArray>()) {
      JsonArray src = d.as<JsonArray>();
      uint8_t n = min((uint8_t)src.size(), (uint8_t)8);
      for (uint8_t i = 0; i < n; i++) {
        JsonObject s = src[i];
        JsonObject u = users[i];
        if (s.containsKey("nm"))     u["nm"]     = s["nm"];
        if (s.containsKey("tp"))     u["tp"]     = s["tp"];
        if (s.containsKey("smsEn"))  u["smsEn"]  = s["smsEn"];
        if (s.containsKey("callEn")) u["callEn"] = s["callEn"];
        if (s.containsKey("en"))     u["en"]     = s["en"];
      }
    }
  }

  size_t len = measureJson(r);
  char* buf = (char*)malloc(len + 1);
  if (!buf) { wsSendErr(c, "contacts", "out of memory"); return; }
  serializeJson(r, buf, len + 1);
  c->text(buf);
  free(buf);
}

// Sends portal format: {respHeader:"remotes", users:[{id, remId}, ...]}
// New strategy: slot number == user number (1..8)
void sendPageRemotes(AsyncWebSocketClient* c)
{
  StaticJsonDocument<512> r;

  r["respHeader"] = "remotes";

  JsonArray users = r.createNestedArray("users");

  for (uint8_t id = 1; id <= 8; id++)
  {
    JsonObject u = users.createNestedObject();
    u["id"] = id;

    uint32_t code = RemoteStorage::getBaseCode(id);

    if (code == 0) {
      // Empty slot
      u["remId"] = "";
    } else {
      char code_str[12];  // enough for 32-bit decimal
      snprintf(code_str, sizeof(code_str), "%lu", (unsigned long)code);
      u["remId"] = code_str;
    }
  }

  char buf[300];
  serializeJson(r, buf, sizeof(buf));
  c->text(buf);
}

void sendPageInfo(AsyncWebSocketClient* c){
  DynamicJsonDocument r(512);
  r["respHeader"] = "info";
  r["P1"] = (myAlarm_pannel.get_system_state()!=DEACTIVE) ? "ACTIVE" : "DEACTIVE";
  r["P9"] = client.connected() ? "CONNECTED" : "DISCONNECTED";
  r["P10"] = getSignal_strength();
  r["P2"] = WiFi.RSSI();
  char ipbuf[16];
  WiFi.localIP().toString().toCharArray(ipbuf, sizeof(ipbuf));
  r["P3"] = ipbuf;
  char macbuf[18];
  WiFi.macAddress().toCharArray(macbuf, sizeof(macbuf));
  r["P4"] = macbuf;
  char buf[300];
  serializeJson(r, buf, sizeof(buf));
  c->text(buf);
}






void handleWebSocketMessage(AsyncWebSocketClient* client, void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo*)arg;

    if (info->opcode != WS_TEXT) {
        return;
    }

    // Start of a new message
    if (info->index == 0) {
        wsReleaseTextBuffer();
        ws_client_id = client->id();
        ws_expected_len = info->len;

        if (ws_expected_len == 0 || ws_expected_len > WS_TEXT_MAX_LEN) {
            wsReleaseTextBuffer();
            wsSendErr(client, "", "msg too large");
            return;
        }

        ws_text_buf = (char*)malloc(ws_expected_len + 1);
        if (ws_text_buf == nullptr) {
            wsReleaseTextBuffer();
            wsSendErr(client, "", "no memory");
            return;
        }
    }

    // Ignore if another client interferes
    if (ws_client_id != client->id()) {
        return;
    }

    // Copy this piece into the correct offset
    if (ws_text_buf == nullptr || (info->index + len) > ws_expected_len) {
        wsSendErr(client, "", "overflow");
        wsReleaseTextBuffer();
        return;
    }

    memcpy(ws_text_buf + info->index, data, len);

    // IMPORTANT: completion check (do NOT rely on info->final)
    bool messageComplete = (info->index + len == ws_expected_len);
    if (!messageComplete) {
        return;
    }

    // Now parse the FULL message
    ws_text_buf[ws_expected_len] = '\0';

    DynamicJsonDocument req(16000);   // big enough for zones JSON
    DeserializationError err = deserializeJson(req, ws_text_buf, ws_expected_len);

    if (err) {
        Serial.print(F("WS JSON error: "));
        Serial.println(err.c_str());
        wsSendErr(client, "", "bad json");
        wsReleaseTextBuffer();
        return;
    }

    Serial.print(F("WS RAW: "));
    Serial.println(ws_text_buf);
// json prity print req
    Serial.print(F("WS JSON: "));
    serializeJsonPretty(req, Serial);

    const char* action = req["action"] | "";
    const char* page   = req["page"] | "";
    Serial.printf("WS OK action=%s page=%s bytes=%u\n", action, page, (unsigned)ws_expected_len);

    // ---- your dispatcher continues here ----

    // ---------------- init ----------------
    if (strcmp(action, "init") == 0) {
        sendPageSys(client);
        sendPageZones(client);
        sendPageContacts(client);
        sendPageInfo(client);
        wsReleaseTextBuffer();
        return;
    }

    // ---------------- get ----------------
    if (strcmp(action, "get") == 0) {

        if (strcmp(page, "sys") == 0)      { sendPageSys(client); wsReleaseTextBuffer(); return; }
        if (strcmp(page, "zones") == 0)    { sendPageZones(client); wsReleaseTextBuffer(); return; }
        if (strcmp(page, "contacts") == 0) { sendPageContacts(client); wsReleaseTextBuffer(); return; }
        if (strcmp(page, "remotes") == 0)  { sendPageRemotes(client); wsReleaseTextBuffer(); return; }
        if (strcmp(page, "info") == 0)     { sendPageInfo(client); wsReleaseTextBuffer(); return; }

        wsSendErr(client, page, "unknown page");
        wsReleaseTextBuffer();
        return;
    }

    // ---------------- set ----------------
    if (strcmp(action, "set") == 0) {

        // ---- set sys ----
        if (strcmp(page, "sys") == 0) {

            const char* errMsg = nullptr;

            if (!saveSystemSettingsFromReq(req, &errMsg)) {
                wsSendErr(client, "sys", errMsg ? errMsg : "Save failed");
                wsReleaseTextBuffer();
                return;
            }

            wsSendOk(client, "sys", "Saved");
            sendPageSys(client);
            wsReleaseTextBuffer();
            return;
        }

        // ---- set zones ----
        if(!strcmp(page, "zones"))
        {
          Serial.println(F("zone page rx"));
            // 1) Validate input
            if (!req.containsKey("zones") || !req["zones"].is<JsonArray>()) {
                wsSendErr(client, "zones", "Missing zones array");
                wsReleaseTextBuffer();
                return;
            }

            JsonArray arr = req["zones"].as<JsonArray>();

            // 2) Ensure zones exist in RAM (creates file if missing/corrupt)
            if (!ZoneStorage::loadOrInit(SPIFFS, "/zones.bin", any_sensor_array, ZONE_COUNT)) {
                wsSendErr(client, "zones", "zones.bin load/init failed");
                wsReleaseTextBuffer();
                return;
            }

            // 3) Apply flags from JSON to RAM
            uint8_t count = arr.size();
            if (count > ZONE_COUNT) count = ZONE_COUNT;

            for (uint8_t i = 0; i < count; i++)
            {
                JsonObject z = arr[i];
                bool by  = z["by"]  | false;
                bool ed  = z["ed"]  | false;
                bool xd  = z["xd"]  | false;
                bool rf  = z["rf"]  | false;
                bool x24 = z["x24"] | false;
                bool sl  = z["sl"]  | false;
                bool pm  = z["pm"]  | false;
                bool ch  = z["ch"]  | false;

                setBit(any_sensor_array[i].device_state, BIT_MASK_BYPASSED,    by);
                setBit(any_sensor_array[i].device_state, BIT_MASK_ENTRY_DELAY, ed);
                setBit(any_sensor_array[i].device_state, BIT_MASK_EXIT_DELAY,  xd);

                setBit(any_sensor_array[i].device_type, BIT_MASK_RF,        rf);
                setBit(any_sensor_array[i].device_type, BIT_MASK_24H,       x24);
                setBit(any_sensor_array[i].device_type, BIT_MASK_SILENT,    sl);
                setBit(any_sensor_array[i].device_type, BIT_MASK_PERIMETER, pm);
                setBit(any_sensor_array[i].device_type, BIT_MASK_CHIME,     ch);
            }

            // 4) Save flags + names in one write (names not kept in RAM)
            struct NameCtx {
                JsonArray zones;
            };

            auto getNameCb = [](uint8_t index, void* user) -> const char*
            {
                NameCtx* ctx = (NameCtx*)user;

                if (ctx == nullptr) return nullptr;
                if (ctx->zones.isNull()) return nullptr;
                if (index >= ctx->zones.size()) return nullptr;

                JsonVariant v = ctx->zones[index]["n"];
                if (v.is<const char*>()) {
                    const char* name = v.as<const char*>();
                    if (name && name[0]) return name;
                }

                return nullptr; // ZoneStorage will fallback to default "Zxx"
            };

            NameCtx ctx;
            ctx.zones = arr;

            bool saved = ZoneStorage::saveWithNameGetter(SPIFFS, "/zones.bin",
                                                        any_sensor_array, ZONE_COUNT,
                                                        getNameCb, &ctx);

            if (!saved) {
                wsSendErr(client, "zones", "zones.bin save failed");
                wsReleaseTextBuffer();
                return;
            }

            // 5) Reply OK and refresh zones page
            wsSendOk(client, "zones", "Saved");
            sendPageZones(client);
            wsReleaseTextBuffer();
            return;
        }

        // ---- set contacts ----
        if (strcmp(page, "contacts") == 0) {
            if (!req.containsKey("users") || !req["users"].is<JsonArray>()) {
                wsSendErr(client, "contacts", "Missing users array");
                wsReleaseTextBuffer();
                return;
            }

            // users.json is a flat root array — no wrapper key, no remID
            DynamicJsonDocument doc(JSON_DOC_SIZE_USER_DATA);
            doc.set(req["users"]);

            if (!writeJsonAtomic("/users.json", doc)) {
                wsSendErr(client, "contacts", "Save failed");
                wsReleaseTextBuffer();
                return;
            }

            wsSendOk(client, "contacts", "Saved");
            sendPageContacts(client);
            wsReleaseTextBuffer();
            return;
        }

        wsSendErr(client, page, "set not implemented yet");
        wsReleaseTextBuffer();
        return;
    }

    // ---------------- cmd ---------------------------------------------------------------------
    
if (strcmp(action, "cmd") == 0) {

    const char* cmd = req["command"] | "";
    StaticJsonDocument<256> r;
    // Use the page field to decide which command set to use
    // (Your portal already sends page in most messages, and req["page"] exists)
    if (strcmp(page, "zones") == 0) {
        // ===== EXISTING ZONE RF COMMANDS (UNCHANGED) =====
        if (strcmp(cmd, "bt0") == 0) {
            uint8_t device_index = req["data0"] | 0;
            r["respHeader"] = "data";
            r["scan_rfid"] = get_device_RFID(device_index);
        }
        else if (strcmp(cmd, "bt1") == 0) {
           
            r["respHeader"] = "ok";
            r["page"] = "zones";
            r["message"] = "RF scan started";
            Serial.printf("WS CMD cmd=%s req=%s\n", cmd, req);
        }
        else if (strcmp(cmd, "bt2") == 0) {
            
            const char* rfid = req["data0"] | "";
            uint8_t zone_id  = req["data1"] | 0;
            char buff[15];
            strlcpy(buff, rfid, sizeof(buff));
            set_device_RFID(zone_id, buff);

            wsSendOk(client, "zones", "RF ID saved");
            sendPageZones(client);
            wsReleaseTextBuffer();
            return;
        }
        else {
            wsSendErr(client, "zones", "unknown zones command");
            wsReleaseTextBuffer();
            return;
        }

        char buf[300];
        serializeJson(r, buf, sizeof(buf));
        client->text(buf);
        wsReleaseTextBuffer();
        return;
    }

    else if (strcmp(page, "remotes") == 0) {
        // ===== NEW REMOTES PAGE COMMANDS =====
        // data0 / data1 usage:
        //  - rm_read_slot:  data0=slot
        //  - rm_scan:       data0=slot (optional)
        //  - rm_save_slot:  data0=code, data1=slot
        //  - rm_clear_slot: data0=slot
        //  - rm_get_user:   data0=userId
        //  - rm_assign_user:data0=userId, data1=slot   (enforce 1:1)

        if (strcmp(cmd, "rm_read_slot") == 0) {
            uint8_t uiSlot = atoi(req["data0"] | "0");
            if (uiSlot < 1 || uiSlot > 8) {
                wsSendErr(client, "remotes", "slot out of range");
                wsReleaseTextBuffer();
                return;
            }
            uint8_t slot = (uint8_t)(uiSlot - 1);
            uint32_t baseCode = RemoteStorage::getBaseCode(slot);
            Serial.printf("[rm_read_slot] uiSlot=%u slot=%u baseCode=%lu\n",
                          uiSlot, slot, (unsigned long)baseCode);

            r["respHeader"] = "data";
            r["page"] = "remotes";
            r["slot"] = uiSlot;
            r["code"] = baseCode;
        }
        else if (strcmp(cmd, "rem_scan") == 0) {

            // Start scan (you already have task + queue system)
            uint8_t uiSlot = atoi(req["data0"] | "0");  // data0 arrives as string
            Serial.printf("cmd=%s uiSlot=%u\n", cmd, uiSlot);
            // Arm scan in RF module (NO task creation here)
            rfScanArm(SCAN_REMOTES, (uiSlot > 0) ? (int8_t)(uiSlot - 1) : -1, 30000); // 30s
            r["respHeader"] = "ok";
            r["page"] = "remotes";
            r["message"] = "Remote scan started";
            r["slot"] = uiSlot;
        }
        else if (strcmp(cmd, "rem_learn") == 0) {

            const char* code = req["data0"] | "";
            const char* slot_char = req["data1"] | "";
            //convert slot_char to uint  
            uint8_t uiSlot = atoi(slot_char);
            Serial.printf("cmd=%s data0=%s data1=%u\n", cmd, code, uiSlot);
            if (uiSlot < 1 || uiSlot > 8) {
                wsSendErr(client, "remotes", "slot out of range");
                wsReleaseTextBuffer();
                return;
            }
            if (code[0] == '\0') {
                wsSendErr(client, "remotes", "empty code");
                wsReleaseTextBuffer();
                return;
            }
            uint8_t slot = (uint8_t)(uiSlot - 1);
            uint32_t baseCode = 0;
            uint8_t cmdCode = 0;
            if (RemoteStorage::extractBaseAndCmd(code, &baseCode, &cmdCode) == 0) {
                wsSendErr(client, "remotes", "invalid remote code");
                wsReleaseTextBuffer();
                return;
            }

            if (!RemoteStorage::learnFromCodeStr(slot, code)) {//learnFromCodeStr(0, "1234567890", true);
                wsSendErr(client, "remotes", "save slot failed");
                wsReleaseTextBuffer();
                return;
            }

            wsSendOk(client, "remotes", "Remote slot saved");
            // optional: send remotes page data back if you have sendPageRemotes()
            // sendPageRemotes(client);
            wsReleaseTextBuffer();
            return;
        }
        else if (strcmp(cmd, "rem_clear") == 0) {

            const char* slot_char = req["data0"] | "";
            uint8_t uiSlot = atoi(slot_char);

            if (uiSlot < 1 || uiSlot > 8) {
                wsSendErr(client, "remotes", "slot out of range");
                wsReleaseTextBuffer();
                return;
            }
            uint8_t slot = (uint8_t)(uiSlot - 1);
            
            if (!RemoteStorage::removeSlot(slot)) {
                wsSendErr(client, "remotes", "clear slot failed");
                wsReleaseTextBuffer();
                return;
            }

            wsSendOk(client, "remotes", "Remote slot cleared");
            wsReleaseTextBuffer();
            return;
        }
        else if (strcmp(cmd, "rm_get_user") == 0) {

            uint8_t userId = req["data0"] | 0;

            r["respHeader"] = "data";
            r["page"] = "remotes";
            r["user"] = userId;
           // r["slot"] = get_user_remote_slot(userId); // 0 = none
        }
        
        else {
            wsSendErr(client, "remotes", "unknown remotes command");
            wsReleaseTextBuffer();
            return;
        }

        char buf[300];
        serializeJson(r, buf, sizeof(buf));
        client->text(buf);
        wsReleaseTextBuffer();
        return;
    }

    // If cmd came without a known page
    wsSendErr(client, "cmd", "unknown cmd page");
    wsReleaseTextBuffer();
    return;
}


    wsSendErr(client, "", "unknown action");
    wsReleaseTextBuffer();
}


// Write JSON atomically: write to temp, then rename
static bool writeJsonAtomic(const char* path, JsonDocument& doc) {
  const char* tmp = "/config.tmp";

  // Remove old temp if exists
  if (SPIFFS.exists(tmp)) SPIFFS.remove(tmp);

  File f = SPIFFS.open(tmp, FILE_WRITE);
  if (!f) return false;

  if (serializeJson(doc, f) == 0) {
    f.close();
    SPIFFS.remove(tmp);
    return false;
  }
  f.close();

  // Replace target
  if (SPIFFS.exists(path)) SPIFFS.remove(path);
  return SPIFFS.rename(tmp, path);
}

/**
 * Save system settings coming from WS request.
 * Expects req["sysconf"] to be an object.
 * Keeps format: { "sysconf": { ... } }
 */
static bool saveSystemSettingsFromReq(JsonDocument& req, const char** errMsgOut) {
  if (!req.containsKey("sysconf") || !req["sysconf"].is<JsonObject>()) {
    if (errMsgOut) *errMsgOut = "Missing sysconf object";
    return false;
  }

  // Load existing config.json if present (so we can preserve other top-level keys if you add later)
  DynamicJsonDocument cfg(8192);  // adjust if your config grows
  if (SPIFFS.exists("/config.json")) {
    File in = SPIFFS.open("/config.json", FILE_READ);
    if (in) {
      DeserializationError e = deserializeJson(cfg, in);
      in.close();
      if (e) {
        // If existing is broken, we can still overwrite cleanly
        cfg.clear();
      }
    }
  }

  // Ensure top-level object
  if (!cfg.is<JsonObject>()) cfg.to<JsonObject>();

  // Merge incoming sysconf fields into flat root (config.json has no wrapper)
  JsonObject incoming = req["sysconf"].as<JsonObject>();
  for (JsonPair kv : incoming) {
    cfg[kv.key()] = kv.value();
  }

  if (!writeJsonAtomic("/config.json", cfg)) {
    if (errMsgOut) *errMsgOut = "Write failed";
    return false;
  }

  return true;
}


