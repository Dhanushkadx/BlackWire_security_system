/*
    Name:       ESP32_websocket.ino
    Created:	11/22/2022 8:06:05 PM
    Author:     DARKLOAD\dhanu
*/
#include "async_web_server.h"

#define TOTAL_DEVICES 8
String hostname = "Digital Security";
TimerSW Timer_WIFIreconnect;
const char* http_username = "admin";
const char* http_password = "admin";

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

void initWiFi_STA(){
	WiFi.mode(WIFI_STA);
	// Configures static IP address
#ifdef define CUSTOM_NETWORK_CONFIG
	if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
		Serial.println(F("STA Failed to configure"));
	}
#endif
#ifdef FORCE_BSSID
		WiFi.begin(structSysConfig.wifipass_sta, structSysConfig.wifipass_sta,6,bssid);
#else
		WiFi.begin(systemConfig.wifissid_sta, systemConfig.wifipass);
		Serial.println(systemConfig.wifipass);
#endif
	Serial.printf("Trying to connect [%s] ", systemConfig.wifissid_sta);
	/*while (WiFi.status() != WL_CONNECTED) {
		Serial.print(".");
		delay(500);
	}*/
	Serial.printf(" %s\n", WiFi.localIP().toString().c_str());
}

void initWiFi_AP() {
	
	
	// Connect to Wi-Fi network with SSID and password
	Serial.println("Setting AP (Access Point)");
	// get the ESP32's MAC address
	uint8_t mac[6];
	WiFi.macAddress(mac);

	// convert the MAC address to a char string
	char macStr[18];  // buffer to hold the MAC address string
	sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	// set the WiFi AP name to "BLACK_WIRE_" followed by the MAC address string
	char apName[30];  // buffer to hold the WiFi AP name
	sprintf(apName, "BLACK_WIRE_%s", macStr);
	WiFi.softAP(apName);

	// print the WiFi AP name
	Serial.print("WiFi AP name: ");
	Serial.println(apName);
	//WiFi.softAP(wifi_ap_name, systemConfig.wifipass, 10, 0, 2);


	IPAddress IP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(IP);
	
}



void reconnect() {
	
	// Loop until we're reconnected
	Timer_WIFIreconnect.previousMillis = millis();
	status = WiFi.status();
	if ( status != WL_CONNECTED) {
#ifdef CUSTOM_NETWORK_CONFIG
		WiFi.setHostname(hostname.c_str()); //define hostname
		if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
			Serial.println(F("STA Failed to configure"));
		}
#endif
		WiFi.begin(systemConfig.wifissid_sta, systemConfig.wifipass);
		Serial.println(F("password>"));
		Serial.println(systemConfig.wifipass);
		while (WiFi.status() != WL_CONNECTED) {
			Serial.println(F("Connecting WiFi..."));
			vTaskDelay(1000 / portTICK_RATE_MS);
			Serial.print(".");
			if (Timer_WIFIreconnect.Timer_run()) {
				WiFi.disconnect();
				//ESP.restart();
				break;
			}
		}
		Serial.println(F("Connected to AP"));
		Serial.printf(" %s\n", WiFi.localIP().toString().c_str());
		Serial.println();
		Timer_WIFIreconnect.previousMillis = millis();
		wifiStarted = true;
	}
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


void onGetRequest_info(AsyncWebServerRequest *request) {}
 // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
void onGetRequest(AsyncWebServerRequest *request) {
	String inputMessage;
	
	if (request->hasParam("z0")) {// I need to know the source web page of the GET request if this para available it s mean page is zone page
		File fileToReadx = SPIFFS.open("/zone_data_8.json") ;
		DynamicJsonDocument docrx(JSON_DOC_SIZE_ZONE_DATA);
		deserializeJson(docrx,  fileToReadx);
		fileToReadx.close();
		for (int index=0; index<TOTAL_DEVICES; index++)
		{
			char buff_cb[5]="cb";	char buff_cbb[5]="b";	char buff_cben[5]="en";	char buff_cbxt[5]="xt";	char buff_cbrf[5]="rf";	char buff_cb24[5]="24";	char buff_cbch[5]="ch";
			char buffer[20]; char buffer2[20];
			sprintf(buffer,"z%d",index);
			if (index < 10) {
				sprintf(buffer2, "z0%d", index);
			}
			else {
				sprintf(buffer2, "z%d", index);
			}
			
			if (request->hasParam(buffer)) {
				inputMessage = request->getParam(buffer)->value();
				docrx[buffer2]["n"]= inputMessage;
			}
			
			char buffer_cbb[10];
			strcpy(buffer_cbb,buffer);
			strcat(buffer_cbb,buff_cb);
			strcat(buffer_cbb,buff_cbb);
			if (request->hasParam(buffer_cbb)) {
				docrx[buffer2]["by"]= true;
			}
			else{
				docrx[buffer2]["by"]= false;
			}
			
			char buffer_cben[10];
			strcpy(buffer_cben,buffer);
			strcat(buffer_cben,buff_cb);
			strcat(buffer_cben,buff_cben);
			if (request->hasParam(buffer_cben)) {
				
				docrx[buffer2]["ed"]= true;
			}
			else{
				docrx[buffer2]["ed"]= false;
			}
			
			char buffer_cbxt[10];
			strcpy(buffer_cbxt,buffer);
			strcat(buffer_cbxt,buff_cb);
			strcat(buffer_cbxt,buff_cbxt);
			if (request->hasParam(buffer_cbxt)) {
				
				
				docrx[buffer2]["xd"]= true;
			}
			else{
				docrx[buffer2]["xd"]= false;
			}
			
			char buffer_cb24[10];
			strcpy(buffer_cb24,buffer);
			strcat(buffer_cb24,buff_cb);
			strcat(buffer_cb24,buff_cb24);
			Serial.println(buff_cb24);
			if (request->hasParam(buffer_cb24)) {
				
				
				docrx[buffer2]["x24"]= true;
			}
			else{
				docrx[buffer2]["x24"]= false;
			}
			
			char buffer_cbrf[10];
			strcpy(buffer_cbrf,buffer);
			strcat(buffer_cbrf,buff_cb);
			strcat(buffer_cbrf,buff_cbrf);
			if (request->hasParam(buffer_cbrf)) {				
				docrx[buffer2]["rf"]= true;				
			}
			else{
				docrx[buffer2]["rf"]= false;				
			}			
			char buffer_cbch[10];
			strcpy(buffer_cbch,buffer);
			strcat(buffer_cbch,buff_cb);
			strcat(buffer_cbch,buff_cbch);
			if (request->hasParam(buffer_cbch)) {
				
				docrx[buffer2]["sl"]= true;
			}
			else{
				docrx[buffer2]["sl"]= false;
			}
		}
		File fileToWritex = SPIFFS.open("/zone_data_8.json", FILE_WRITE);
		
		serializeJson(docrx,  fileToWritex);
		serializeJsonPretty(docrx, Serial); 
		fileToWritex.close();
		request->send(200, "text/text", "OK");
	}
	
if (request->hasParam("tp1")) {
	File fileToReady = SPIFFS.open("/personx.json");
	DynamicJsonDocument docry(JSON_DOC_SIZE_USER_DATA);
	deserializeJson(docry,  fileToReady);
	fileToReady.close();
	for (int index=1; index<=TOTAL_PHONE_NUMBER_COUNT; index++)
	{
		char buffer[10]; char buffer2[10]; char buffer3[10];
		
		sprintf(buffer,"tp%d",index);		
		if (request->hasParam(buffer)) {
			inputMessage = request->getParam(buffer)->value();
			Serial.println(buffer);
			Serial.println(inputMessage);
			
			sprintf(buffer2, "P%d", index);
			
			docry[buffer2]["number"]= inputMessage;
		}
		memset(buffer,'\0',10);
		memset(buffer3,'\0',10);
		sprintf(buffer,"SMS%d",index);
		sprintf(buffer3,"CALL%d",index);
		if (request->hasParam(buffer))
		{
			docry[buffer2]["sms"]= "1";
		}
		else{
			docry[buffer2]["sms"]= "0";
		}
		if (request->hasParam(buffer3))
		{
			docry[buffer2]["call"]= "1";
		}
		else{
			docry[buffer2]["call"]= "0";
		}
	}
	File fileToWritey = SPIFFS.open("/personx.json", FILE_WRITE);
	
	serializeJson(docry,  fileToWritey);
	serializeJsonPretty(docry, Serial);
	fileToReady.close();
	request->send(200, "text/text", "OK");
	 
}	
	
	
	
if (request->hasParam("txt0")) {
	File fileToReadz = SPIFFS.open("/config.json");
	DynamicJsonDocument docrz(2048);
	deserializeJson(docrz,  fileToReadz);
	fileToReadz.close();
	
	/* "entry_delay_time"*/
	
	if (request->hasParam("txt0")) {
		inputMessage = request->getParam("txt0")->value();
		docrz["sysconf"]["entry_delay_time"]= inputMessage;
		
	}
	/*"et_en"*/
	if (request->hasParam("cb0")) {
		docrz["sysconf"]["et_en"]= true;
	}
	else{
		docrz["sysconf"]["et_en"]= false;
	}
	/*"et_beep"*/
	if (request->hasParam("cb1")) {
		docrz["sysconf"]["et_beep"]= true;
	}
	else{
		docrz["sysconf"]["et_beep"]= false;
	}
	/*"exit_delay_time"*/
	if (request->hasParam("txt1")) {
		inputMessage = request->getParam("txt1")->value();
		docrz["sysconf"]["exit_delay_time"]= inputMessage;
		
	}
	/*"xt_en"*/
	if (request->hasParam("cb2")) {
		docrz["sysconf"]["xt_en"]= true;
	}
	else{
		docrz["sysconf"]["xt_en"]= false;
	}
	/*"xt_beep"*/
	if (request->hasParam("cb3")) {
		docrz["sysconf"]["xt_beep"]= true;
	}
	else{
		docrz["sysconf"]["xt_beep"]= false;
	}
	
	/*"beep duration"*/
	if (request->hasParam("txt2")) {
		inputMessage = request->getParam("txt2")->value();
		docrz["sysconf"]["beep_time_out"]= inputMessage;
		
	} 
	/*"siren duration"*/
	if (request->hasParam("txt8")) {
		inputMessage = request->getParam("txt8")->value();
		docrz["sysconf"]["bell_time_out"]= inputMessage;
		
	}     

	/*"beep_en"*/
	if (request->hasParam("cb8")) {
		docrz["sysconf"]["siren_en"]= true;
	}
	else{
		docrz["sysconf"]["siren_en"]= false;
	}

	/*"siren_en"*/
	if (request->hasParam("cb4")) {
		docrz["sysconf"]["beep_en"]= true;
	}
	else{
		docrz["sysconf"]["beep_en"]= false;
	}	
	 // calling attempts
	 if (request->hasParam("list0")) {
            inputMessage = request->getParam("list0")->value();
            Serial.printf("Selected call attempt value:%s",inputMessage.c_str());
			docrz["sysconf"]["call_attempts"] = inputMessage;
     }
	 /*"alrm call Enable"*/
	if (request->hasParam("cb5")) {
		docrz["sysconf"]["call_en"]= true;
	}
	else{
		docrz["sysconf"]["call_en"]= false;
	}
	
	/*"wifissid_sta"*/
	if (request->hasParam("txt3")) {
		inputMessage = request->getParam("txt3")->value();
		docrz["sysconf"]["wifissid_sta"]= inputMessage;
		
	}
	/*"WiFi STA Enable"*/
	if (request->hasParam("cb6")) {
		docrz["sysconf"]["wifi_sta_en"]= true;
	}
	else{
		docrz["sysconf"]["wifi_sta_en"]= false;
	}
	/*"wifipass"*/
	if (request->hasParam("psw0")) {
		inputMessage = request->getParam("psw0")->value();
		docrz["sysconf"]["wifipass"]= inputMessage;
		
	}
	//MQTT_server
	if (request->hasParam("txt5")) {
		inputMessage = request->getParam("txt5")->value();
		docrz["sysconf"]["mqtt_server"]= inputMessage;
		
	}
	/*"MQTT Enable"*/
	if (request->hasParam("cb7")) {
		docrz["sysconf"]["mqtt_en"]= true;
	}
	else{
		docrz["sysconf"]["mqtt_en"]= false;
	}
	//MQTT_port
	if (request->hasParam("txt6")) {
		inputMessage = request->getParam("txt6")->value();
		docrz["sysconf"]["mqtt_port"]= inputMessage;
		
	}
	//MQTT_user
	if (request->hasParam("txt7")) {
		inputMessage = request->getParam("txt7")->value();
		docrz["sysconf"]["mqtt_user"]= inputMessage;
		
	}
	//MQTT_pass
	if (request->hasParam("psw2")) {
		inputMessage = request->getParam("psw2")->value();
		docrz["sysconf"]["mqtt_pass"]= inputMessage;
		
	}
	
	/*"wifi_sta_en"*/
	if (request->hasParam("cb6")) {	
		docrz["sysconf"]["wifi_sta_en"]= true;
	}
	else{
		docrz["sysconf"]["wifi_sta_en"]= false;
	}
	/*"installer_no"*/
	if (request->hasParam("txt4")) {
		inputMessage = request->getParam("txt4")->value();
		docrz["sysconf"]["installer_no"]= inputMessage;
		
	}
	
	/*"installer_pass"*/
	if (request->hasParam("psw1")) {
		inputMessage = request->getParam("psw1")->value();
		docrz["sysconf"]["installer_pass"]= inputMessage;
		
	}
	
	File fileToWritez = SPIFFS.open("/config.json", FILE_WRITE);	
	serializeJson(docrz,  fileToWritez);
	fileToReadz.close();
}	
// reload config
     eeprom_load(2);
	 request->send(200, "text/text", "OK");
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
	 if(path == "/") {
		 path = "/index.html";
	 }
	 request->send(SPIFFS, path, "text/html", false, processor);
}


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



