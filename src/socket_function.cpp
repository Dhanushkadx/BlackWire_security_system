// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------
#include "socket_function.h"

// If you keep zone names in a JSON file, set this to 1
#define USE_ZONE_NAMES_JSON  1

AsyncWebSocket ws("/ws");


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
  if(!ZoneStorage::load(SPIFFS, "/zones.bin", any_sensor_array, ZONE_COUNT)){
    // still send something, otherwise UI stays empty
  }
  DynamicJsonDocument r(8192);
  r["respHeader"] = "zones";
  JsonArray arr = r.createNestedArray("zones");

  for(uint8_t i=0;i<ZONE_COUNT;i++){
    JsonObject z = arr.createNestedObject();
    // n/by/ed/xd/rf/x24/sl
    z["n"]   = /* name */ String("Z") + (i<10?"0":"") + String(i);
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
    r[key] = getSensor(i);
  }

  String out; serializeJson(r, out);
  c->text(out);
}

// ---- Main dispatcher ----
void handleWebSocketMessage(AsyncWebSocketClient* client, void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if(!(info->final && info->index==0 && info->len==len && info->opcode==WS_TEXT)) return;

  DynamicJsonDocument req(len + 128);
  auto err = deserializeJson(req, data, len);
  if(err) return wsSendErr(client, "", "bad json");

  const char* action = req["action"] | "";
  const char* page   = req["page"]   | "";

  if(!strcmp(action,"init")){
    // send all pages (portal says init = request all) :contentReference[oaicite:8]{index=8}
    sendPageSys(client);
    sendPageZones(client);
    sendPageContacts(client);
    sendPageInfo(client);
    // sendPageLog(client); // if you implement
    return;
  }

  if(!strcmp(action,"get")){
    if(!strcmp(page,"sys"))      return sendPageSys(client);
    if(!strcmp(page,"zones"))    return sendPageZones(client);
    if(!strcmp(page,"contacts")) return sendPageContacts(client);
    if(!strcmp(page,"info"))     return sendPageInfo(client);
    if(!strcmp(page,"log"))      {/* sendPageLog(client); */ return;}
    return wsSendErr(client, page, "unknown page");
  }

  if(!strcmp(action,"set")){
    // TODO: implement saves (config.json / zones.bin / personx.json)
    // On success: {respHeader:"ok", page:"...", message:"OK"} :contentReference[oaicite:9]{index=9}
    // On fail:    {respHeader:"err", page:"...", message:"reason"} :contentReference[oaicite:10]{index=10}
	const char* page = req["page"] | "";

		if(!strcmp(page, "sys")) {
			const char* errMsg = nullptr;

			if (!saveSystemSettingsFromReq(req, &errMsg)) {
			wsSendErr(client, "sys", errMsg ? errMsg : "Save failed");
			return;
			}

			wsSendOk(client, "sys", "Saved");

			// Optional: immediately push back what was saved (keeps UI in sync)
			sendPageSys(client);
			return;
		}
		return wsSendErr(client, page, "set not implemented yet");
		// Later you can add:
		// else if(!strcmp(page,"zones")) { ... }
		// else if(!strcmp(page,"contacts")) { ... }

		wsSendErr(client, page, "Unknown set page");
		return;
		
  }

  if(!strcmp(action,"cmd")){
    const char* cmd = req["command"] | "";
    // keep your bt0/bt1/bt2 logic, but reply with respHeader:"data" or "ok"
    StaticJsonDocument<256> r;

    if(!strcmp(cmd,"bt0")){
      uint8_t device_index = req["data0"] | 0;
      r["respHeader"] = "data";
      r["scan_rfid"] = get_device_RFID(device_index);
    }else if(!strcmp(cmd,"bt1")){
      xTaskCreate(Task6code,"Task6",5000,NULL,6,&Task6);
      r["respHeader"] = "ok";
      r["page"] = "zones";
      r["message"] = "RF scan started";
    }else if(!strcmp(cmd,"bt2")){
		const char* rfid = req["data0"] | "";
		uint8_t zone_id  = req["data1"] | 0;
		char buff[15];
		strlcpy(buff, rfid, sizeof(buff));
		set_device_RFID(zone_id, buff);
		wsSendOk(client, "zones", "RF ID saved");
		// optional: send updated zones to refresh UI
		sendPageZones(client);

		return;
}else{
      return wsSendErr(client, "cmd", "unknown command");
    }

    String out; serializeJson(r, out);
    client->text(out);
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


