// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------
#include "socket_function.h"

// If you keep zone names in a JSON file, set this to 1
#define USE_ZONE_NAMES_JSON  1

AsyncWebSocket ws("/ws");

static size_t   ws_text_len = 0;
static uint32_t ws_text_client = 0;
static char ws_text_buf[12000];  // make it bigger than your zones JSON
static uint32_t ws_client_id = 0;
static size_t ws_expected_len = 0;


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
  String out; serializeJson(r, out);
  c->text(out);
}

static void wsSendOk(AsyncWebSocketClient* c, const char* page, const char* msg){
  StaticJsonDocument<192> r;
  r["respHeader"] = "ok";
  r["page"] = page ? page : "";
  r["message"] = msg;
  String out; serializeJson(r, out);
  c->text(out);
}

// ---- send pages (modify your existing notifyClients_* to accept client*) ----
void sendPageSys(AsyncWebSocketClient* c){
  File f = SPIFFS.open("/config.json", FILE_READ);
  if(!f) { wsSendErr(c, "sys", "config.json open failed"); return; }

  DynamicJsonDocument d(JSON_DOC_SIZE_CONFIG_DATA);
  DeserializationError err = deserializeJson(d, f);
  f.close();
  if(err) { wsSendErr(c, "sys", "config.json parse failed"); return; }

  // Portal expects: { respHeader:"sys", sysconf:{...} }
  StaticJsonDocument<64> head;
  DynamicJsonDocument r(JSON_DOC_SIZE_CONFIG_DATA + 96);

  r["respHeader"] = "sys";

  if (d.containsKey("sysconf")) {
    // Your file format (recommended)
    r["sysconf"] = d["sysconf"];
  } else {
    // Fallback: if old format was flat JSON
    r["sysconf"] = d.as<JsonObject>();
  }

  String out;
  serializeJson(r, out);
  c->text(out);
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
  DynamicJsonDocument r(8192);
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
    z["sl"]  = (any_sensor_array[i].device_type  & (1 << BIT_MASK_SILENT)) != 0; // UI label: Chime
    // optional: z["status"] = ...
  }

  String out; serializeJson(r, out);
  c->text(out);
}

void sendPageContacts(AsyncWebSocketClient* c){
  File f = SPIFFS.open("/personx.json", FILE_READ);
  if(!f) return wsSendErr(c, "contacts", "personx.json open failed");

  DynamicJsonDocument d(JSON_DOC_SIZE_USER_DATA);
  auto err = deserializeJson(d, f);
  f.close();
  if(err) return wsSendErr(c, "contacts", "personx.json parse failed");

  // Ensure portal format: {respHeader:"contacts", users:[...]}
  // If your file already contains "users", just wrap/forward it.
  DynamicJsonDocument r(JSON_DOC_SIZE_USER_DATA + 64);
  r["respHeader"] = "contacts";
  if (d.containsKey("users")) r["users"] = d["users"];
  else if (d.is<JsonArray>()) r["users"] = d.as<JsonArray>();
  else r["users"] = d.as<JsonObject>(); // last resort (better to normalize)

  String out; serializeJson(r, out);
  c->text(out);
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

  String out;
  serializeJson(r, out);
  c->text(out);
}

void sendPageInfo(AsyncWebSocketClient* c){
  DynamicJsonDocument r(768);
  r["respHeader"] = "info";
  // fill P1.. etc from your existing notifyClients_pageInfo()
  r["P1"] = (myAlarm_pannel.get_system_state()!=DEACTIVE) ? "ACTIVE" : "DEACTIVE";
  r["P9"] = client.connected() ? "CONNECTED" : "DISCONNECTED";
  r["P10"] = getSignal_strength();
  r["P2"] = WiFi.RSSI();
  r["P3"] = WiFi.localIP().toString();
  r["P4"] = WiFi.macAddress();
  for(int i=0;i<4;i++){
    String key = "P" + String(i+5);
    r[key] = true;//getSensor(i);
  }

  String out; serializeJson(r, out);
  c->text(out);
}






void handleWebSocketMessage(AsyncWebSocketClient* client, void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo*)arg;

    if (info->opcode != WS_TEXT) {
        return;
    }

    // Start of a new message
    if (info->index == 0) {
    ws_client_id = client->id();
    ws_expected_len = info->len;

    if (ws_expected_len >= sizeof(ws_text_buf)) {
        ws_expected_len = 0;
        wsSendErr(client, "", "msg too large");
        return;
    }
}

    // Ignore if another client interferes
    if (ws_client_id != client->id()) {
        return;
    }

    // Copy this piece into the correct offset
    if (info->index + len > sizeof(ws_text_buf)) {
        wsSendErr(client, "", "overflow");
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
        // Now parse the FULL message  
        ws_expected_len = 0;

        return;
    }

    Serial.print("WS RAW: ");
    Serial.println(ws_text_buf);
// json prity print req
    Serial.print("WS JSON: ");
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
        return;
    }

    // ---------------- get ----------------
    if (strcmp(action, "get") == 0) {

        if (strcmp(page, "sys") == 0)      { sendPageSys(client); return; }
        if (strcmp(page, "zones") == 0)    { sendPageZones(client); return; }
        if (strcmp(page, "contacts") == 0) { sendPageContacts(client); return; }
        if (strcmp(page, "remotes") == 0) {  sendPageRemotes(client); return; }
        if (strcmp(page, "info") == 0)     { sendPageInfo(client); return; }

        wsSendErr(client, page, "unknown page");
        return;
    }

    // ---------------- set ----------------
    if (strcmp(action, "set") == 0) {

        // ---- set sys ----
        if (strcmp(page, "sys") == 0) {

            const char* errMsg = nullptr;

            if (!saveSystemSettingsFromReq(req, &errMsg)) {
                wsSendErr(client, "sys", errMsg ? errMsg : "Save failed");
                return;
            }

            wsSendOk(client, "sys", "Saved");
            sendPageSys(client);
            return;
        }

        // ---- set zones ----
        if(!strcmp(page, "zones"))
        {
          Serial.println("zone page rx");
            // 1) Validate input
            if (!req.containsKey("zones") || !req["zones"].is<JsonArray>()) {
                wsSendErr(client, "zones", "Missing zones array");
                return;
            }

            JsonArray arr = req["zones"].as<JsonArray>();

            // 2) Ensure zones exist in RAM (creates file if missing/corrupt)
            if (!ZoneStorage::loadOrInit(SPIFFS, "/zones.bin", any_sensor_array, ZONE_COUNT)) {
                wsSendErr(client, "zones", "zones.bin load/init failed");
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

                setBit(any_sensor_array[i].device_state, BIT_MASK_BYPASSED,    by);
                setBit(any_sensor_array[i].device_state, BIT_MASK_ENTRY_DELAY, ed);
                setBit(any_sensor_array[i].device_state, BIT_MASK_EXIT_DELAY,  xd);

                setBit(any_sensor_array[i].device_type,  BIT_MASK_RF,     rf);
                setBit(any_sensor_array[i].device_type,  BIT_MASK_24H,    x24);
                setBit(any_sensor_array[i].device_type,  BIT_MASK_SILENT, sl);
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
                return;
            }

            // 5) Reply OK and refresh zones page
            wsSendOk(client, "zones", "Saved");
            sendPageZones(client);
            return;
        }

        wsSendErr(client, page, "set not implemented yet");
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
            return;
        }
        else {
            wsSendErr(client, "zones", "unknown zones command");
            return;
        }

        String out;
        serializeJson(r, out);
        client->text(out);
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
            uint8_t slot = req["data0"] | 0;

            r["respHeader"] = "data";
            r["page"] = "remotes";
            r["slot"] = slot;
            r["code"] = RemoteStorage::getBaseCode(slot); // "" if empty
        }
        else if (strcmp(cmd, "rem_scan") == 0) {

            // Start scan (you already have task + queue system)
            uint8_t slot = req["data0"] | 0;   // 1..8 expected (or 0)
            Serial.printf("cmd=%s slot=%u\n", cmd, slot);
            // Arm scan in RF module (NO task creation here)
            rfScanArm(SCAN_REMOTES, (slot > 0) ? (int8_t)slot : -1, 30000); // 30s
            r["respHeader"] = "ok";
            r["page"] = "remotes";
            r["message"] = "Remote scan started";
            r["slot"] = slot;
        }
        else if (strcmp(cmd, "rem_learn") == 0) {

            const char* code = req["data0"] | "";
            const char* slot_char = req["data1"] | "";
            //convert slot_char to uint  
            uint8_t slot = atoi(slot_char);
           Serial.printf("cmd=%s data0=%s data1=%u\n", cmd, code, slot);
            if (slot < 1 || slot > 8) {
                wsSendErr(client, "remotes", "slot out of range");
                return;
            }
            if (!code) {
                wsSendErr(client, "remotes", "empty code");
                return;
            }

            if (!RemoteStorage::learnFromCodeStr(slot, code)) {//learnFromCodeStr(0, "1234567890", true);
                wsSendErr(client, "remotes", "save slot failed");
                return;
            }

            wsSendOk(client, "remotes", "Remote slot saved");
            // optional: send remotes page data back if you have sendPageRemotes()
            // sendPageRemotes(client);
            return;
        }
        else if (strcmp(cmd, "rem_clear") == 0) {

            const char* slot_char = req["data0"] | "";
            uint8_t slot = atoi(slot_char);

            if (slot < 1 || slot > 8) {
                wsSendErr(client, "remotes", "slot out of range");
                return;
            }
            
            if (!RemoteStorage::removeSlot(slot)) {
                wsSendErr(client, "remotes", "clear slot failed");
                return;
            }

            wsSendOk(client, "remotes", "Remote slot cleared");
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
            return;
        }

        String out;
        serializeJson(r, out);
        client->text(out);
        return;
    }

    // If cmd came without a known page
    wsSendErr(client, "cmd", "unknown cmd page");
    return;
}


    wsSendErr(client, "", "unknown action");
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

  // Replace sysconf completely with incoming sysconf
  cfg["sysconf"] = req["sysconf"];

  // OPTIONAL: enforce some safety defaults (example)
  // cfg["sysconf"]["xtDelay"] = max(0, (int)cfg["sysconf"]["xtDelay"]);
  // cfg["sysconf"]["enDelay"] = max(0, (int)cfg["sysconf"]["enDelay"]);

  if (!writeJsonAtomic("/config.json", cfg)) {
    if (errMsgOut) *errMsgOut = "Write failed";
    return false;
  }

  return true;
}


