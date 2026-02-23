/*
    Name:       ESP32_websocket.ino
    Created:	11/22/2022 8:06:05 PM
    Author:     DARKLOAD\dhanu
*/
#include "async_web_server.h"


// ---------- Optional: keep these somewhere global ----------
static bool wifiEventsRegistered = false;

#define TOTAL_DEVICES 8
String hostname = "Digital Security";
TimerSW Timer_WIFIreconnect;
const char* http_username = "admin";
const char* http_password = "admin";


#define KEY_BYPASS   "cbb"
#define KEY_ENTRY    "cben"
#define KEY_EXIT     "cbxt"
#define KEY_RF       "cbrf"
#define KEY_24H      "cb24"
#define KEY_SILENT   "cbch"

//#define CUSTOM_NETWORK_CONFIG
// the IP address for the shield:
// Set your Static IP address
IPAddress local_IP(10, 0, 0, 100);
//IPAddress local_IP(192,168,43,10);
// Set your Gateway IP address
IPAddress gateway(10, 0, 0, 1);

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

long previousMillis =0;
long interval = 30000;
#define HTTP_PORT 80

// wifi
// the Wifi radio's status
int status = WL_IDLE_STATUS;
bool wifiStarted = false;
bool  setup_web_server_started = false;


StaticJsonDocument<JSON_DOC_SIZE_DEVICE_DATA> docz;

AsyncWebServer server(HTTP_PORT);

// --------- helper: build keys safely ----------
static inline void makeKey(char* out, size_t outSz, int idx, const char* suffix) {
  // suffix examples: "" , "cbb", "cen", "cxt", "crf", "c24", "cch", "cpm"
  snprintf(out, outSz, "z%d%s", idx, suffix);
}

// --------- helper: set/clear bit ----------
static inline void setBit(uint8_t &v, uint8_t bit, bool en) {
  if (en) v |=  (1U << bit);
  else    v &= ~(1U << bit);
}


void setup_web_server_with_AP()
{
	Serial.println(F("setting WiFi-AP"));
	initWiFi_AP();	
	initWebSocket();
	initWebServer();
	

}

void setup_web_server_with_STA()
{
	Serial.println(F("setting WiFi-STA"));
	initWiFi_STA();	
	initWebSocket();
	initWebServer();
}

void setup_web_server_with_STA_info()
{
	Serial.println(F("setting WiFi-STA Info"));
	initWiFi_STA();	
	initWebSocket();
	initWebServer_info();
}

void cleanClients(){
	ws.cleanupClients();
}



void initWebSocket() {
	ws.onEvent(onEvent);
	server.addHandler(&ws);
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
	Serial.println(F("Connected to AP successfully!"));

  }
  
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
	Serial.printf_P(PSTR("\nConnected to %s\n"), systemConfig.wifissid_sta);
		delay(3000);
		char IP[] = "xxx.xxx.xxx.xxx";          // buffer
		IPAddress ip = WiFi.localIP();
		String my_ip = ip.toString();
		Serial.print(F("IP: "));
		Serial.println(my_ip.c_str());
		//initRTC();
		wifiStarted = true;
		uint32_t colour = Adafruit_NeoPixel::Color(0, 0, 255);
  		pixel.startBlink(colour, 100, 1000, 255);
  }
  
  void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
	Serial.println(F("Disconnected from WiFi access point"));
	Serial.print(F("Reason: "));
	Serial.println(info.wifi_sta_disconnected.reason);
	Serial.println(F("Trying to Reconnect"));
	WiFi.reconnect();
	if(wifiStarted){// Loop until we're reconnected
		Timer_WIFIreconnect.previousMillis = millis();
		wifiStarted = false;
		uint32_t red = Adafruit_NeoPixel::Color(0, 0, 255);
  		pixel.startBlink(red, 300, 300, 180);
		
	}
	
		vTaskDelay(500 / portTICK_RATE_MS);
			if (Timer_WIFIreconnect.Timer_run()) {
				Serial.println(F("WiFi connection timeout"));
				//WiFi.disconnect();
				//ESP.restart();
				return;
			}
  }



// ----------------------------------------------------------
// Register WiFi events ONCE (no duplicates)
static void registerWiFiEventsOnce() {
  if (wifiEventsRegistered) return;

  WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  wifiEventsRegistered = true;
}

// ----------------------------------------------------------
// Cleanly stop previous WiFi state before switching modes
static void wifiStopAll() {
  WiFi.disconnect(true, true); // erase old STA connection + stop
  WiFi.softAPdisconnect(true); // stop AP
  delay(50);
}

// ----------------------------------------------------------
// STA init
void initWiFi_STA()
{
  registerWiFiEventsOnce();
  wifiStopAll();

  WiFi.mode(WIFI_STA);

  // Optional: STA hostname (helps router list)
  // WiFi.setHostname("PrimeHive-Panel");

  // Optional: reduce reconnection delay behavior
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);  // don't write creds to flash automatically

#ifdef CUSTOM_NETWORK_CONFIG   // ✅ correct usage
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println(F("STA failed to configure static IP"));
  }
#endif

#ifdef FORCE_BSSID
  // NOTE: WiFi.begin(ssid, pass, channel, bssid, connect)
  WiFi.begin(systemConfig.wifissid_sta, systemConfig.wifipass, 6, bssid, true);
#else
  WiFi.begin(systemConfig.wifissid_sta, systemConfig.wifipass);
#endif

  Serial.printf_P(PSTR("Trying STA connect [%s]\n"), systemConfig.wifissid_sta);

  uint32_t blue = Adafruit_NeoPixel::Color(0, 0, 255);
  pixel.startBlink(blue, 300, 300, 180);
}



void initWiFi_AP()
{
  wifiStopAll();

  Serial.println(F("Setting AP (Access Point)"));
  WiFi.mode(WIFI_AP);

  uint8_t mac[6];
  WiFi.macAddress(mac);

  char apName[32];
  // Example: BLACK_WIRE_A1B2C3D4E5F6
  snprintf(apName, sizeof(apName),
           "BLACK_WIRE_%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // If you want open AP (no password): pass nullptr
  // Better: use password (8+ chars required)
  const char* apPass = systemConfig.wifipass;  // or fixed "12345678" during setup
  const int   channel = 6;
  const bool  hidden = false;
  const int   maxConn = 2;

  bool ok;
  if (apPass && strlen(apPass) >= 8) {
    ok = WiFi.softAP(apName, apPass, channel, hidden, maxConn);
  } else {
    // fallback open AP if password invalid
    ok = WiFi.softAP(apName, nullptr, channel, hidden, maxConn);
    Serial.println(F("AP password invalid (<8). Started OPEN AP."));
  }

  if (!ok) {
    Serial.println(F("Failed to start AP"));
    return;
  }

  Serial.print(F("WiFi AP name: "));
  Serial.println(apName);

  IPAddress IP = WiFi.softAPIP();
  Serial.print(F("AP IP address: "));
  Serial.println(IP);

  uint32_t yellow = Adafruit_NeoPixel::Color(255, 255, 0);
  pixel.startBlink(yellow, 1000, 1000, 255);
}






void initSPIFFS() {
	Serial.println(F("init SPIFF"));
	if (!SPIFFS.begin()) {
		Serial.println("Cannot mount SPIFFS volume...");
		while (1) {
			delay(100);
		}
	}
	
	
}

// ----------------------------------------------------------------------------
// Web server initialization
// ----------------------------------------------------------------------------

String processor(const String &var) {
	return String(var == "STATE" ? "on" : "off");
}



// =====================================================
// ZONES submit handler
// marker param: "z0"
// expects checkbox keys: z<idx>cbb, z<idx>cen, z<idx>cxt, z<idx>crf, z<idx>c24, z<idx>cch
// =====================================================
static bool handleZonesSubmit(AsyncWebServerRequest *request)
{
  if (!request->hasParam("z0")) return false;

  // (Optional) ensure current is loaded. If you always keep array in RAM, you can remove.
  (void)ZoneStorage::load(SPIFFS, "/zones.bin", any_sensor_array, TOTAL_DEVICES);

  char key[16];

  for (int idx = 0; idx < TOTAL_DEVICES; idx++) {
    makeKey(key, sizeof(key), idx, KEY_BYPASS);
    setBit(any_sensor_array[idx].device_state, BIT_MASK_BYPASSED, request->hasParam(key));

    makeKey(key, sizeof(key), idx, KEY_ENTRY);
    setBit(any_sensor_array[idx].device_state, BIT_MASK_ENTRY_DELAY, request->hasParam(key));

    makeKey(key, sizeof(key), idx, KEY_EXIT);
    setBit(any_sensor_array[idx].device_state, BIT_MASK_EXIT_DELAY, request->hasParam(key));

    makeKey(key, sizeof(key), idx, KEY_RF);
    setBit(any_sensor_array[idx].device_type, BIT_MASK_RF, request->hasParam(key));

    makeKey(key, sizeof(key), idx, KEY_24H);
    setBit(any_sensor_array[idx].device_type, BIT_MASK_24H, request->hasParam(key));

    makeKey(key, sizeof(key), idx, KEY_SILENT);
    setBit(any_sensor_array[idx].device_type, BIT_MASK_SILENT, request->hasParam(key));

    // Optional timestamp update (enable if you want)
    // any_sensor_array[idx].last_updated_time_stamp = millis();
  }

  if (!ZoneStorage::save(SPIFFS, "/zones.bin", any_sensor_array, ZONE_COUNT)) {
    request->send(500, "text/plain", "FAIL: save zones.bin");
    return true;
  }

  request->send(200, "text/plain", "OK");
  return true;
}

// =====================================================
// PHONE submit handler
// marker param: "tp1"
// updates /personx.json
// =====================================================
static bool handlePhonesSubmit(AsyncWebServerRequest *request)
{
  if (!request->hasParam("tp1")) return false;

  File fileToRead = SPIFFS.open("/personx.json", FILE_READ);
  if (!fileToRead) {
    request->send(500, "text/plain", "FAIL: open /personx.json");
    return true;
  }

  DynamicJsonDocument doc(JSON_DOC_SIZE_USER_DATA);
  DeserializationError err = deserializeJson(doc, fileToRead);
  fileToRead.close();

  if (err) {
    request->send(500, "text/plain", "FAIL: parse /personx.json");
    return true;
  }

  for (int index = 1; index <= TOTAL_PHONE_NUMBER_COUNT; index++) {
    char tpKey[10];   // "tp1"
    char pKey[10];    // "P1"
    char smsKey[10];  // "SMS1"
    char callKey[10]; // "CALL1"

    snprintf(tpKey, sizeof(tpKey), "tp%d", index);
    snprintf(pKey,  sizeof(pKey),  "P%d",  index);
    snprintf(smsKey,sizeof(smsKey),"SMS%d",index);
    snprintf(callKey,sizeof(callKey),"CALL%d",index);

    if (request->hasParam(tpKey)) {
      String v = request->getParam(tpKey)->value();
      doc[pKey]["number"] = v;
    }

    doc[pKey]["sms"]  = request->hasParam(smsKey);
    doc[pKey]["call"] = request->hasParam(callKey);
  }

  File fileToWrite = SPIFFS.open("/personx.json", FILE_WRITE);
  if (!fileToWrite) {
    request->send(500, "text/plain", "FAIL: write /personx.json");
    return true;
  }

  if (serializeJson(doc, fileToWrite) == 0) {
    fileToWrite.close();
    request->send(500, "text/plain", "FAIL: serialize /personx.json");
    return true;
  }
  fileToWrite.close();

#ifdef _DEBUG
  serializeJsonPretty(doc, Serial);
#endif

  request->send(200, "text/plain", "OK");
  return true;
}

// =====================================================
// CONFIG submit handler
// marker param: "txt0"
// updates /config.json then reloads config (eeprom_load(2))
// =====================================================
static bool handleConfigSubmit(AsyncWebServerRequest *request)
{
  if (!request->hasParam("txt0")) return false;

  File fileToRead = SPIFFS.open("/config.json", FILE_READ);
  if (!fileToRead) {
    request->send(500, "text/plain", "FAIL: open /config.json");
    return true;
  }

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, fileToRead);
  fileToRead.close();

  if (err) {
    request->send(500, "text/plain", "FAIL: parse /config.json");
    return true;
  }

  auto sys = doc["sysconf"];

  // ---------- entry delay time ----------
  if (request->hasParam("txt0")) sys["entry_delay_time"] = request->getParam("txt0")->value();
  sys["et_en"]   = request->hasParam("cb0");
  sys["et_beep"] = request->hasParam("cb1");

  // ---------- exit delay time ----------
  if (request->hasParam("txt1")) sys["exit_delay_time"] = request->getParam("txt1")->value();
  sys["xt_en"]   = request->hasParam("cb2");
  sys["xt_beep"] = request->hasParam("cb3");

  // ---------- durations ----------
  if (request->hasParam("txt2")) sys["beep_time_out"] = request->getParam("txt2")->value();
  if (request->hasParam("txt8")) sys["bell_time_out"] = request->getParam("txt8")->value();

  // NOTE: your original code swapped these names. I keep your original mapping:
  // cb8 -> siren_en, cb4 -> beep_en
  sys["siren_en"] = request->hasParam("cb8");
  sys["beep_en"]  = request->hasParam("cb4");

  // ---------- call attempts ----------
  if (request->hasParam("list0")) sys["call_attempts"] = request->getParam("list0")->value();

  // ---------- call enable ----------
  sys["call_en"] = request->hasParam("cb5");

  // ---------- wifi creds ----------
  if (request->hasParam("txt3")) sys["wifissid_sta"] = request->getParam("txt3")->value();
  sys["wifi_sta_en"] = request->hasParam("cb6");
  if (request->hasParam("psw0")) sys["wifipass"] = request->getParam("psw0")->value();

  // ---------- mqtt ----------
  if (request->hasParam("txt5")) sys["mqtt_server"] = request->getParam("txt5")->value();
  sys["mqtt_en"] = request->hasParam("cb7");
  if (request->hasParam("txt6")) sys["mqtt_port"] = request->getParam("txt6")->value();
  if (request->hasParam("txt7")) sys["mqtt_user"] = request->getParam("txt7")->value();
  if (request->hasParam("psw2")) sys["mqtt_pass"] = request->getParam("psw2")->value();

  // ---------- installer ----------
  if (request->hasParam("txt4")) sys["installer_no"] = request->getParam("txt4")->value();
  if (request->hasParam("psw1")) sys["installer_pass"] = request->getParam("psw1")->value();

  // Save config.json
  File fileToWrite = SPIFFS.open("/config.json", FILE_WRITE);
  if (!fileToWrite) {
    request->send(500, "text/plain", "FAIL: write /config.json");
    return true;
  }

  if (serializeJson(doc, fileToWrite) == 0) {
    fileToWrite.close();
    request->send(500, "text/plain", "FAIL: serialize /config.json");
    return true;
  }
  fileToWrite.close();

#ifdef _DEBUG
  serializeJsonPretty(doc, Serial);
#endif

  // reload config (your original had eeprom_load(2) after saving)
  eeprom_load(2);

  request->send(200, "text/plain", "OK");
  return true;
}

// =====================================================
// MAIN dispatcher: exactly one response per request
// =====================================================
void onGetRequest(AsyncWebServerRequest *request)
{
  if (handleZonesSubmit(request))  return;
  if (handlePhonesSubmit(request)) return;
  if (handleConfigSubmit(request)) return;

  request->send(400, "text/plain", "Unknown request");
}

 
 void onRootRequest_info(AsyncWebServerRequest *request) {
	if(!request->authenticate(http_username, systemConfig.installer_pass))
	return request->requestAuthentication();	 
	String path = request->url();
	if(path == "/") {
		path = "/info.html";
	}
	request->send(SPIFFS, path, "text/html", false, processor);
}

void onRootRequest(AsyncWebServerRequest *request) {
  if(!request->authenticate(http_username, systemConfig.installer_pass))
    return request->requestAuthentication();

  String path = request->url();
  if (path == "/") path = "/index.html";   // your portal renamed as index.html

  request->send(SPIFFS, path, "text/html"); // ✅ NO processor here
}

// void onRootRequest(AsyncWebServerRequest *request) {
// 	 if(!request->authenticate(http_username, systemConfig.installer_pass))
// 	 return request->requestAuthentication();	 
// 	 String path = request->url();
// 	 if(path == "/") {
// 		 path = "/index.html";
// 	 }
// 	 request->send(SPIFFS, path, "text/html", false, processor);
// }


void initWebServer() {
	
	server.on("/", onRootRequest);
	server.on("/get", onGetRequest);
	server.serveStatic("/", SPIFFS, "/");
	AsyncElegantOTA.begin(&server);    // Start ElegantOTA
	server.begin();
	/*server
	.serveStatic("/", SPIFFS, "/www/")
	.setDefaultFile("default.html")
	.setAuthentication("user", "pass");*/
	server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
  	request->send(204);
});
}

void initWebServer_info() {
	
	//server.on("/", onRootRequest_info);
	//server.on("/get", onGetRequest_info);
	//server.serveStatic("/", SPIFFS, "/");
	//AsyncElegantOTA.begin(&server);    // Start ElegantOTA
	//server.begin();
	/*server*/
	 // send a file when /index is requested
	 server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/info.html");
	  });
	  server.begin();
	//server.setDefaultFile("info.html");
	//server.setAuthentication("user", "pass");
}



