#include <Arduino.h>
#include "Wire.h"
/*
    Name:       ESP32_gsm_alarm.ino
    Created:	10/30/2022 8:48:33 AM
    Author:     DARKLOAD\dhanu
*/
#define _DEBUG
#define ARDUINOJSON_ENABLE_STD_STREAM 1

#include <freertos/message_buffer.h>
#include "pinsx.h"
#include <ESPmDNS.h>
#include "FS.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>
//#include <fstream>
#include "WiFi.h"
//#include "ESPAsyncWebServer.h"
#include "typex.h"
#include "Text_msg.h"
#include "statments.h"
#include "alarm.h"
#include "TimerSW.h"
#include <RCSwitch.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "ESP32Time.h"
#include "gsm_broker.h"
#include "rf_methods.h"
#include "sensor_scan.h"
#include "config_manager.h"
#include "async_web_server.h"

#ifdef MQTT_OK
#include "mqtt_broker.h"
#endif
//ESP32Time rtc;
ESP32Time rtc(-3600);  // offset in seconds GMT+1

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;
const int   daylightOffset_sec = 3600;

SemaphoreHandle_t xBinarySemaphore;
SemaphoreHandle_t xMutex_GSM = NULL;
SemaphoreHandle_t xMutex_GSM_CALLING = NULL;
SemaphoreHandle_t xMutex_I2C = NULL;
SemaphoreHandle_t sensorMutex = NULL;


/* this variable hold queue handle */
xQueueHandle xQueue;
/* this variable hold queue handle */
xQueueHandle xQueue_sensor_state;
xQueueHandle xQueue_mqtt_Qhdlr;
/* create event group */
EventGroupHandle_t EventRTOS_lcd;
EventGroupHandle_t EventRTOS_lcdkeyPad;
EventGroupHandle_t EventRTOS_gsm;
EventGroupHandle_t EventRTOS_buzzer;

 size_t xBufferSizeBytes = 100;
MessageBufferHandle_t xMessageBuffer;

size_t xBufferSizeBytes_number = 20;
MessageBufferHandle_t xMessageBuffer_number;

MessageBufferHandle_t xMessageBuffer_zone;
size_t xBufferSizeBytes_zone = 25;

MessageBufferHandle_t xTimeBuffer;
 size_t xTimeBufferSizeBytes = 50;

char pcStringToSend[50]=">";
char TimeToSend_buff[50]="";

//using json = nlohmann::json;
TimerSW Timer_rf_id_auto_clr;
RCSwitch mySwitch = RCSwitch();
LiquidCrystal_I2C lcd(0x3C,16,2);

TaskHandle_t Task1;
TaskHandle_t Task2_sms;
TaskHandle_t Task3;
TaskHandle_t Task4;
TaskHandle_t Task5;
TaskHandle_t Task6;
TaskHandle_t Task7;
TaskHandle_t Task8;
TaskHandle_t Task9;
TaskHandle_t Task10;


// Use this one for FONA 3G
//Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);

//uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
void rfid_int_to_str(char* buff, unsigned long rf_id);
void transfer_rf_scan_data(char* rfid);
void RFbaster(const char* rf_id_msg);
void RFListiner();
void Power_detect_loop(); 
void buzzer();
void printLocalTime();
void send_all_zone_states_mqtt();
uint8_t remcode(const char* codeStr);
uint8_t type;

TimerSW Timer_call_init_delay;
TimerSW Timer_call_answer_delay;
TimerSW Timer_sms_send_delay;
TimerSW Timer_sms_netwok_busy_delay;
TimerSW Timer_sms_read_interal;
TimerSW Timer_sys_update;
TimerSW Timer_keypad_lcd_event;
TimerSW Timer_battery_charge;
TimerSW Timer_alarm_calling_time_out;
TimerSW Timer_mqtt_breath;

eSYS_MODE system_mode = NOMAL_MODE_NO_WIFI;
ALARM myAlarm_pannel(any_sensor_array);

//typedef enum power_states{YES,NO,UNK};
bool ac_main_power_current_state  = 0;
bool ac_main_power_prev_state  = 0;
const uint16_t detectDelay = 500; // delay between power state readings (in milliseconds)
const long stableDuration = 2000; // duration of stable power state (in milliseconds)
boolean lcd_refresh = true;
bool rf_id_automatic_clr_timer_en= true;


EVENT_INFO STRUCT_event_infor;
MY_SENS any_sensor_array[TOTAL_DEVICES];
systemConfigTypedef_struct systemConfig;
//String inputString;
String inputString = "";         // a string to hold incoming data
boolean stringComplete_at_serial0 = false;  // whether the string is complete
char netowrk_operator_name_char[10];
unsigned long prev_RFID=0;

boolean lcd_update = true;
byte infor[TOTAL_DEVICES*2];

unsigned long try_time = 0;

int v_bat=0;


eInvoking_source last_invorker = PSERIAL0;
uint8_t scan_type;

	
eMain_state eCurrent_state = DEACTIVE;
eMain_state ePrev_state = BEGING;

SYS_STATE eCrrunt_lcd_page= DISARM, ePrev_lcd_page=BEGING_LCD, eBackup_lcd_page = DISARM;

char mode_change_source[10]="System";
//LCD buffers
char lcd_faulty_sensors[25];
char lcd_system_states[25];
//scrolling
//----------------------------------
/*
int Li          = 16;
int Lii         = 0;
int Ri          = -1;
int Rii         = -1;
*/


void Task1code( void * parameter ){
	/*Serial.print("Task1 is running on core ");
	Serial.println(xPortGetCoreID());*/
	//xSemaphoreTake( xMutex_GSM_CALLING, portMAX_DELAY );
	/* keep the status of receiving data */
	TickType_t xLastWakeTime;
    const TickType_t xFrequency =  pdMS_TO_TICKS(1);;
	BaseType_t xStatus_queu_sensorTsk, xStatus_queu_mqttTsk;
	/* time to block the task until data is available */
	const TickType_t xTicksToWait = pdMS_TO_TICKS(10);
	DataBuffer data_buff_sensTsk, data_buff_mqttTsk;
	for(;;){
		vTaskDelayUntil( &xLastWakeTime, xFrequency );		
		/* receive data from the queue */
		xStatus_queu_sensorTsk = xQueueReceive( xQueue_sensor_state, &data_buff_sensTsk, xTicksToWait );
		/* check whether receiving is ok or not */
		if(xStatus_queu_sensorTsk == pdPASS){
			/* print the data to terminal */
			Serial.print(F("rx Queu sensor data: "));
			Serial.println(data_buff_sensTsk.char_buffer_rx);
			universal_event_hadler(data_buff_sensTsk.char_buffer_rx,SYSTEM_ITSELF,0);
		}
#ifdef MQTT_OK
		xStatus_queu_mqttTsk = xQueueReceive( xQueue_mqtt_Qhdlr, &data_buff_mqttTsk, xTicksToWait );
		/* check whether receiving is ok or not */
		if(xStatus_queu_mqttTsk == pdPASS){
			/* print the data to terminal */
			Serial.print(F("mqtt Queu data: "));
			Serial.println(data_buff_mqttTsk.char_buffer_rx);
			universal_event_hadler(data_buff_mqttTsk.char_buffer_rx,WEB,0);
		}
#endif		
		myAlarm_pannel.watcher();
		
		if (stringComplete_at_serial0)
		{
			stringComplete_at_serial0 = false;
			universal_event_hadler(inputString.c_str(),last_invorker,0);
			inputString = "";
		}
		
		RFListiner();
	}
}

void Task2code_sms( void * parameter ){
	/*Serial.print("Task2 is running on core ");
	Serial.println(xPortGetCoreID());*/
	creatSMS("Hi :-)",3,0);
 TickType_t xLastWakeTime;
 const TickType_t xFrequency =  pdMS_TO_TICKS(1);;
 // Initialize the xLastWakeTime variable with the current time.
 xLastWakeTime = xTaskGetTickCount ();
 for(;;){
		vTaskDelayUntil( &xLastWakeTime, xFrequency );		
#ifdef GSM_OK	
		ultimate_sms_hadlr();
#endif
	}
}

void Task3code_lcd( void * parameter ){
	/*Serial.print("Task3 is running on core ");
	Serial.println(xPortGetCoreID());*/
	 TickType_t xLastWakeTime;
	 const TickType_t xFrequency = pdMS_TO_TICKS(1000);
	// Initialise the xLastWakeTime variable with the current time.
	 xLastWakeTime = xTaskGetTickCount ();
	 
	for(;;){
		
		 vTaskDelayUntil( &xLastWakeTime, xFrequency );
		 
		//nomal_mode_lcd_hdrl();
		
		 /*if (wifiStarted) {
			 if (!setup_web_server_started)
			 {
				  setup_web_server();
				  setup_web_server_started = true;
			 }
			
		 }*/
		 
	}
}

void Task4code_gsm_ctrl( void * parameter ){
	/*Serial.print("Task4 is running on core ");
	Serial.println(xPortGetCoreID());*/
	 TickType_t xLastWakeTime;
	 const TickType_t xFrequency =  pdMS_TO_TICKS(3000);
	 // Initialize the xLastWakeTime variable with the current time.
	 xLastWakeTime = xTaskGetTickCount ();
	for(;;){
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
#ifdef GSM_OK		
			gsm_manager();		
#endif
	}
}

void Task5code_call( void * parameter ){
	/*Serial.print("Task5 is running on core ");
	Serial.println(xPortGetCoreID());*/
	 TickType_t xLastWakeTime;
	 const TickType_t xFrequency =  pdMS_TO_TICKS(500);;
	 // Initialize the xLastWakeTime variable with the current time.
	 xLastWakeTime = xTaskGetTickCount ();
	for(;;){
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
#ifdef GSM_OK	
		ultimate_call_hadlr();		
#endif
	}
}

void Task6code(void * parameter){
	/* keep the status of receiving data */
	BaseType_t xStatus;
	/* time to block the task until data is available */
	const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
	DataBuffer data;
	Serial.print(F("task 6 start"));
	for(;;){
		/* receive data from the queue */
		xStatus = xQueueReceive( xQueue, &data, xTicksToWait );
		/* check whether receiving is ok or not */
		if(xStatus == pdPASS){
			/* print the data to terminal */
			
			Serial.print(F("rfid = "));
			Serial.print(data.char_buffer_rx);
			Serial.println(data.counter);
			DynamicJsonDocument json(250);
			json["respHeader"]= "data";
			json["scan_rfid"] = data.char_buffer_rx+5;
			String data_str;
			size_t len = serializeJson(json, data_str);
			Serial.print(F("send json and free heap>"));
			Serial.println(ESP.getFreeHeap());
			notifyClients_rfidRx(data_str.c_str(),len);
			break;
		}
	}
	Serial.print(F("delete task 6"));
	vTaskDelete( NULL );
}

void Task7code( void * parameter ){
	/*Serial.print("Task7 is running on core ");
	Serial.println(xPortGetCoreID());*/
	
	for(;;){
		delay(1);
		//loop_keyPad();
	}
}

void Task8code( void * parameter ){
	/*Serial.print("Task8 is running on core ");
	Serial.println(xPortGetCoreID());*/
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = pdMS_TO_TICKS(50);
	esp_task_wdt_init(30, true);
	for(;;){
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
	

switch (system_mode) {
    case NOMAL_MODE_WIFI: {
#ifdef MQTT_OK
        mqtt_com_loop();
#endif
        reconnect();
        if (Timer_mqtt_breath.Timer_run()) {
            publish_network_info();
            send_all_zone_states_mqtt();
            Timer_mqtt_breath.previousMillis = millis();
        }
    } break;

    case NOMAL_MODE_NO_WIFI: { // Assuming you meant a different mode here
       
    } break;

	case CONFIG_MODE: { // Assuming you meant a different mode here
       
    } break;

    default:
        // Handle other modes or errors
        break;
}
}
}


void Task9code( void * parameter ){
	Serial.print(F("Task9 is running on core "));
	Serial.println(xPortGetCoreID());
	
	for(;;){
		delay(1);
		buzzer();
	}
}

void Task10code( void * parameter ){
	Serial.print(F("Task10 is running on core "));
	Serial.println(xPortGetCoreID());
	TickType_t xLastWakeTime;
	const TickType_t xFrequency =  pdMS_TO_TICKS(10);;
	// Initialize the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount ();
	for(;;){
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
		GPIO_sens_scan();
		pooling_i2c_a();
		pooling_i2c_b();
		pooling_i2c_c();
		Power_detect_loop();
		if (Timer_battery_charge.Timer_run())
		{
			Serial.println(F("BAT CHAR OFF"));
			digitalWrite(PIN_BATTERY,LOW);
		}
	}
}

void init_timersSW(){
	 //Timer_second_counter.interval = 1000;
	  Timer_WIFIreconnect.interval = 60000;
	  Timer_keypad_lcd_event.interval = 10000;
	  Timer_battery_charge.interval = 5000000;
	  Timer_sms_send_delay.interval = 60000;
	  Timer_mqtt_breath.interval = 30000;
}

void setup()
{
	pinMode(PIN_AC_DETECT,INPUT_PULLUP);
	//pinMode(PROGRAM_PIN,INPUT_PULLUP);
	pinMode(BuzzerPin,OUTPUT);
	pinMode(PIN_RF_LED,OUTPUT);
	pinMode(PIN_GSM_BUSY_LED,OUTPUT);
	pinMode(GSM_LED,OUTPUT);
	digitalWrite(BuzzerPin,LOW);
	pinMode(RELAY_ALARM , OUTPUT);
	pinMode(PIN_ARM , OUTPUT);
	pinMode(PIN_DISARM, OUTPUT);
	pinMode(PIN_BATTERY, OUTPUT);
	pinMode(PIN_VOICE_EN,OUTPUT);
	
	Wire.begin();
	uint8_t dx;
	init_timersSW();
	Serial.begin(115200);
	Serial.print(F("Smart Security Alarm System Rev 2.0"));
	//lcd.init();

	//lcd.backlight();
	//lcd.print("INITILIZING...");
	//init_keyPad();
   // Init and get the time
   configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
   printLocalTime();
   
	// Initialize SPIFFS
	if(!SPIFFS.begin(true)){
		Serial.println("An Error has occurred while mounting SPIFFS");
		return;
	}
	//eeprom_reset();
	eeprom_load();
	setup_sensor_settings();
	//system_mode = CONFIG_MODE;
	if(system_mode==CONFIG_MODE){
		setup_web_server_with_AP();
	}
	else if(system_mode==NOMAL_MODE_WIFI){
		setup_web_server_with_STA();
#ifdef MQTT_OK
		if(systemConfig.mqtt_en){mqtt_enable = true; setup_mqtt();}
		else{mqtt_enable = false;}
#endif
	}
	else if(system_mode==NOMAL_MODE_NO_WIFI){//
	Serial.println(F("NO WiFi mode"));
	mqtt_enable = false;
	}

	setup_call_backs();
#ifdef GSM_OK	
	//gsm_init();
#endif
  myAlarm_pannel.set_arm_mode(AS_ITIS_NO_BYPASS);
  myAlarm_pannel.set_system_state(DEACTIVE,SYSTEM_ITSELF,0);
  mySwitch.enableReceive(digitalPinToInterrupt(15));  // Receiver on interrupt 0 => that is pin #2*/
  /* initialize binary semaphore */
  xBinarySemaphore = xSemaphoreCreateBinary();
  xMutex_GSM = xSemaphoreCreateMutex(); 
  xMutex_GSM_CALLING = xSemaphoreCreateMutex();
  xMutex_I2C = xSemaphoreCreateMutex();
  sensorMutex = xSemaphoreCreateMutex();
  EventRTOS_lcd = xEventGroupCreate();
  EventRTOS_buzzer = xEventGroupCreate();
  EventRTOS_lcdkeyPad = xEventGroupCreate();
  EventRTOS_gsm = xEventGroupCreate();
 /* create the queue which size can contains 5 elements of Data */
 xQueue = xQueueCreate(1, sizeof(DataBuffer));
 xQueue_sensor_state = xQueueCreate(1, sizeof(DataBuffer));
 xQueue_mqtt_Qhdlr = xQueueCreate(3, sizeof(DataBuffer));
 if (!xQueue || !xQueue_sensor_state || !xQueue_mqtt_Qhdlr) {
    // Code inside this block will execute if any of the queue creations failed
	Serial.println(F("ini queu faild system can't start"));
}
 
xMessageBuffer = xMessageBufferCreate(xBufferSizeBytes );
xMessageBuffer_number = xMessageBufferCreate(xBufferSizeBytes_number );
xMessageBuffer_zone = xMessageBufferCreate(xBufferSizeBytes_zone );
xTimeBuffer = xMessageBufferCreate(xTimeBufferSizeBytes);
 
	xTaskCreatePinnedToCore(Task1code,"Task1",5000,NULL,1,&Task1,0);	
	xTaskCreatePinnedToCore(Task3code_lcd,"Task3",5000,NULL,3,&Task3,0);
	xTaskCreatePinnedToCore(Task4code_gsm_ctrl,"Task4",5000,NULL,4,&Task4,1);
	xTaskCreatePinnedToCore(Task7code,"Task7",5000,NULL,5,&Task7,1);
	xTaskCreatePinnedToCore(Task8code,"Task8",10000,NULL,1,&Task8,0);
	xTaskCreatePinnedToCore(Task2code_sms,"Task2",10000,NULL,2,&Task2_sms,1);
	
	xTaskCreatePinnedToCore(Task9code,"Task9",1000,NULL,1,&Task9,1);
	xTaskCreatePinnedToCore(Task10code,"Task10",3000,NULL,1,&Task10,1);
	/* Clear bit 0 and bit 4 in xEventGroup. */
	xEventGroupSetBits(
	EventRTOS_gsm,  /* The event group being updated. */
	TASK_4_BIT);/* The bits being cleared. */
	vTaskSuspend(Task2_sms);
}

void loop()
{
	if(system_mode!=NOMAL_MODE_NO_WIFI){
	cleanClients();}
/*
delay(1000);
	 printLocalTime();*/
}
void printLocalTime(){
	struct tm timeinfo;
	if(!getLocalTime(&timeinfo)){
		Serial.println("Failed to obtain time");
		return;
	}
	
#ifdef _DEBUG
	Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
	Serial.print("Day of week: ");
	Serial.println(&timeinfo, "%A");
	Serial.print("Month: ");
	Serial.println(&timeinfo, "%B");
	Serial.print("Day of Month: ");
	Serial.println(&timeinfo, "%d");
	Serial.print("Year: ");
	Serial.println(&timeinfo, "%Y");
	Serial.print("Hour: ");
	Serial.println(&timeinfo, "%H");
	Serial.print("Hour (12 hour format): ");
	Serial.println(&timeinfo, "%I");
	Serial.print("Minute: ");
	Serial.println(&timeinfo, "%M");
	Serial.print("Second: ");
	Serial.println(&timeinfo, "%S");

	Serial.println("Time variables");
	char timeHour[3];
	strftime(timeHour,3, "%H", &timeinfo);
	Serial.println(timeHour);
	char timeWeekDay[10];
	strftime(timeWeekDay,10, "%A", &timeinfo);
	Serial.println(timeWeekDay);
	Serial.println();
#endif
}


void transfer_rf_scan_data(char* rfid){
	 /* keep the status of sending data */
	 BaseType_t xStatus;
	 /* time to block the task until the queue has free space */
	 const TickType_t xTicksToWait = pdMS_TO_TICKS(1);
	 /* create data to send */
	DataBuffer data;
	/* sender 1 has id is 1 */
	memset(data.char_buffer_rx,'\0',15);
	strcpy(data.char_buffer_rx,rfid);
	data.counter = 1;
	
		Serial.println(F("sendTask RFscan is sending data"));
		/* send data to front of the queue */
		xStatus = xQueueSendToFront( xQueue, &data, xTicksToWait );
		/* check whether sending is ok or not */
		if( xStatus == pdPASS ) {
			/* increase counter of sender 1 */
			data.counter = data.counter + 1;
			Serial.println(F("sendTask1 is sending data"));
		}
		/* we delay here so that receiveTask has chance to receive data */
		delay(100);
}


void eeprom_save(){configSave();}
void eeprom_load(){ configLoad();
	myAlarm_pannel.set_entry_delay_timer_interval(systemConfig.entry_delay_time);
	myAlarm_pannel.set_exit_delay_timer_interval(systemConfig.exit_delay_time);
	myAlarm_pannel.set_bell_time_timer_interval(systemConfig.bell_time_out);
 }
void eeprom_reset(){configReset(); configLoad();}

		
		
uint8_t initiliz_sensor_data(){
	
	Serial.println(F("RF zones clearing"));
	
	// update rf sensors as closed	
	for (int x =0; x<TOTAL_DEVICES;x++)
	{
		
		if (myAlarm_pannel.is_sensor_RF(x))// RF zones should always closed 
		{
			myAlarm_pannel.Universal_zone_state_update(x,false);
		}
		
	}	
	uint8_t ret_value=0;
// 	/myAlarm_pannel.Universal_zone_state_update(0,1);

	return ret_value;
}



void serialEventRun() {
	
	while (Serial.available()) {
		// get the new byte:
		char inChar = (char)Serial.read();
		// add it to the inputString:
		if (inChar != '\n')
		inputString += inChar;
		// if the incoming character is a newline, set a flag
		// so the main loop can do something about it:
		if (inChar == '\n') {
			stringComplete_at_serial0 = true;
			// update invoker
			//invoker_rec();			
		}
	}
	
	
}

byte universal_event_hadler(const char* smsbuffer, eInvoking_source Invoker, uint8_t user_id){//0- gsm 1-lcd
	byte ret_value = 0;
	Serial.print(F("Run CLI.. _cliString>"));
	Serial.print(smsbuffer);
	Serial.print(F(" Access level: "));
	Serial.println(systemConfig.cli_access_level);
	Serial.print(F(" User: "));
	Serial.println(user_id);
	Serial.print(F(" Invoker: "));
	
	switch (Invoker)
	{
		case GSM_MODULE:Serial.println(F("GSM"));
		break;
		case PSERIAL0:Serial.println(F("SERIAL0"));
		break;
		case LCD:Serial.println(F("LCD"));
		break;
		case CARD_A:Serial.println(F("CARD A"));
		break;
		case CARD_B:Serial.println(F("CARD B"));
		break;
		case CARD_C:Serial.println(F("CARD C"));
		break;
		case APP:Serial.println(F("App"));
		break;
	}
	
	if (strncmp("reboot",smsbuffer,6)==0)
	{
		delay(5000);
		Serial.println(F("REStart in 5 sec"));
		ESP.restart();
	}
	
	if (strncmp("net?",smsbuffer,4)==0)
	{
		
		Serial.print(F("Network>"));
		Serial.println(netowrk_operator_name_char);
	}
	if (strncmp("info?",smsbuffer,5)==0)
	{
		
		
	}
	if (strncmp("BLOCK",smsbuffer,5)==0)
	{
		while(1){}
		
	}
	
	
	if (strncmp("psw=1234",smsbuffer,3)==0)
	{
		ret_value = 1;
		char psw_char[6];
		strlcpy(psw_char,smsbuffer+4,5);
		int psw_int = atoi(psw_char);
		Serial.println(F("password>"));
		Serial.println(psw_int);
		if (psw_int==1234)
		{
			systemConfig.cli_access_level=3;
			Serial.println(F("cli unlocked"));
		}
		else{
			systemConfig.cli_access_level=0;
			Serial.println(F("cli locked"));
		}
	}
	if (strncmp_P(smsbuffer,PSTR("Name="),4)==0)// Name=01,Front D
	{
		ret_value = 1;
		//check access
		//if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3);}
		//Serial.print(F("zone id>> "));
		char zone_index[5]={0};
		char zone_name[10]={0};
		strlcpy	(zone_index,smsbuffer+5,3);
		uint8_t zone_index_int = atoi(zone_index);		
		strcpy	(zone_name,smsbuffer+8);		
		set_device_name(zone_index_int, zone_name);
		Serial.printf_P(PSTR("**zone renaming**\n"));
		if (Invoker==GSM_MODULE)
		{
			char buffer[5];
			strcpy_P(buffer,PSTR("OK"));
			creatSMS(buffer,2,0);
		}
	}	
	
	if (strncmp_P(smsbuffer,PSTR("Zone="),4)==0)// "Zone=03,EXIT,0";
	{
		set_zone_param(smsbuffer);
		 eeprom_load();
		 char reply_buff[5];
		 strcpy_P(reply_buff,PSTR("OK"));
		 creatSMS(reply_buff,2,0);
	}
	if (strncmp("Status?",smsbuffer,7)==0)
	{
		ret_value = 1;
		char msg[150] = {0};
		String ipAddress = WiFi.localIP().toString();
  		String macAddress = WiFi.macAddress();
  		int32_t rssi = WiFi.RSSI();
		String wifiStatus;
		(!WiFi.isConnected())?wifiStatus = "offline": wifiStatus = "online";
  		sprintf(msg, "WiFi:%s\nSSID:%s\nPW:%s\nIP:%s\nMAC:%s\nRSSI:%d dBm\n",wifiStatus,systemConfig.wifissid_sta,systemConfig.wifipass, ipAddress.c_str(), macAddress.c_str(), rssi);
		//sms_broad_cast_request = true;
		SMS_to_be_sent_FIXDMEM[sms_buffer_msg_count].type = 1;
		if (myAlarm_pannel.get_system_state()!=DEACTIVE)
		{
			strcat_P(msg,PSTR("Mode: ARMED"));
			creatSMS(msg,3,0);
		}
		else
		{
			strcat_P(msg,PSTR("Mode: DISARMED"));
			creatSMS(msg,3,0);
		}
		Serial.println(msg);
		
	}
	if (strncmp_P(smsbuffer,PSTR("Home arm"),8)==0)
	{
		ret_value = 1;
		myAlarm_pannel.set_arm_mode(AS_ITIS_NO_BYPASS);
		myAlarm_pannel.set_system_state(SYS1_IDEAL,Invoker,user_id);
#ifdef MQTT_OK
		publish_system_state("ARMED","info/mode",true);
#endif       
		
	}
	if (strncmp_P(smsbuffer,PSTR("Home arm"),8)==0)
	{
		ret_value = 1;
		myAlarm_pannel.set_arm_mode(AS_ITIS_NO_BYPASS);
		myAlarm_pannel.set_system_state(SYS1_IDEAL,Invoker,user_id);
#ifdef MQTT_OK
		publish_system_state("ARMED","info/mode",true);
#endif       
		
	}
	
	else if(strncmp_P(smsbuffer,PSTR("Disarm"),6)==0){
		ret_value = 1;	
		eCurrent_state=DEACTIVE;
		myAlarm_pannel.set_system_state(DEACTIVE,Invoker,user_id);
#ifdef MQTT_OK
		publish_system_state("DISARMED","info/mode",true);
#endif        
		
	}
	
	else if(strncmp("Panic",smsbuffer,5)==0)
	{
		ret_value =1;
		myAlarm_pannel.set_system_state(PANIC,Invoker,user_id);
	}

	else if(strncmp("Alarm_call",smsbuffer,10)==0)
	{
		ret_value =1;
		//myAlarm_pannel.set_system_state(PANIC,Invoker,user_id);
		myAlarm_pannel.set_system_state(ALARM_CALLING,WEB,0);
#ifdef MQTT_OK
		publish_system_state("ALARM","info/mode",true);
#endif   
	}
	
	else if(strncmp("Number",smsbuffer,6)==0)//Number=1,+94714427691
	{
		ret_value = 1;
		//check access
		//if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3); }
		
		 char number[25];
		 memset(number,'\0',25);
		 strcpy(number,smsbuffer+7);
		 char *index_str = strtok(number, ",");
		 if (index_str == NULL) {
			 Serial.println(F("Error: Invalid input string."));
			 ret_value = 0;
			 return ret_value;
			
		 }

		 char *phone_num_str = strtok(NULL, ",");
		 if (phone_num_str == NULL) {
			 Serial.println(F("Error: Invalid input string."));
			  ret_value = 0;
			  return ret_value;
		 }

		 int tp_number_index = atoi(index_str);
		 if (tp_number_index == 0 && index_str[0] != '0') {
			 Serial.println(F("Error: Invalid zone_index_int value."));
			  ret_value = 0;
			  return ret_value;
		 }

		 Serial.print(F("Index:"));
		 Serial.println(tp_number_index);
		 Serial.print(F("Phone number:"));
		 Serial.println(phone_num_str);
		 char reply_sms[159];
		 memset(reply_sms,'\0',sizeof(reply_sms));
		 char phone_number_char[13];
		 memset(phone_number_char,0, sizeof(phone_number_char));
		 strlcpy(phone_number_char,phone_num_str,13);
		if (phone_number_validat(phone_number_char))
		{
			set_GSM_number(tp_number_index, phone_number_char);
			strcat	(reply_sms,"Numbers");
			strcat	(reply_sms,"\n");
			strcat	(reply_sms,get_GSM_number(1));
			strcat	(reply_sms,"\n");
			strcat	(reply_sms,get_GSM_number(2));
			strcat	(reply_sms,"\n");
			strcat	(reply_sms,get_GSM_number(3));
			strcat	(reply_sms,"\n");
			strcat	(reply_sms,get_GSM_number(4));
			strcat	(reply_sms,"\n");
			strcat	(reply_sms,get_GSM_number(5));
			strcat	(reply_sms,"\n");
			strcat	(reply_sms,get_GSM_number(6));
			strcat	(reply_sms,"\n");
			strcat	(reply_sms,get_GSM_number(7));
			strcat	(reply_sms,"\n");
			strcat	(reply_sms,get_GSM_number(8));
			
		} 
		else
		{
			strcat_P(reply_sms,PSTR("Invalide number format. Eg:+94712345890"));
		}
		
		creatSMS(reply_sms,1,0);
		
		
		
		
		//sms_broad_cast_request=true;
	}
	else if(strncmp_P(smsbuffer,PSTR("SMS number="),10)==0){// set sms number eg: Sms number=01,1
		ret_value = 1;
		//check access
		if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3,0);}
		
		char* eq_start = strstr(smsbuffer,"=");
		char zone_index[10];
		char zone_name[5];
		
		strlcpy(zone_index,eq_start+1,3);
		int tp_number_index = atoi(zone_index);
		Serial.print(F("TP number:"));
		Serial.print(zone_index);
		strlcpy(zone_name,eq_start+4,2);
		int state = atoi(zone_name);
		if (state)
		{
			set_GSM_number_is_sms(tp_number_index,true);
			Serial.println(F("sms enable"));
		}
		else
		{
			set_GSM_number_is_sms(tp_number_index,false);
			Serial.println(F("sms disable"));
		}
		
		eeprom_save();
		if (Invoker==GSM_MODULE)
		{
			char buffer[5];
			strcpy_P(buffer,PSTR("OK"));
			creatSMS(buffer,2,0);
		}
	}
	else if(strncmp_P(smsbuffer,PSTR("CALL number="),11)==0){// set call number
		ret_value = 1;
		//check access
		if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3,0); }
		
		char* eq_start = strstr(smsbuffer,"=");
		char zone_index[10];
		char zone_name[5];
		
		strlcpy(zone_index,eq_start+1,3);
		int tp_number_index = atoi(zone_index);
		Serial.print(F("TP number:"));
		Serial.print(zone_index);
		Serial.print(F("="));
		strlcpy(zone_name,eq_start+4,2);
		int state = atoi(zone_name);
		eeprom_save();
		if (state)
		{
			set_GSM_number_is_call(tp_number_index,true);
			Serial.println(F("call enable"));
		}
		else
		{
			set_GSM_number_is_call(tp_number_index,false);
			Serial.println(F("call disable"));
		}
		if (Invoker==GSM_MODULE)
		{
			char buffer[5];
			strcpy_P(buffer,PSTR("OK"));
			creatSMS(buffer,2,0);
		}
	}
	
	
	else if(strncmp("eerstm2",smsbuffer,7)==0){//remote B
		
		
	}
	else if (strncmp("RID=", smsbuffer, 4) == 0) {//remote B
	ret_value = 1;
	//RFbaster(smsbuffer);
	//send_rfid_state_update_to_mqtt(smsbuffer);
	}
	
	else if(strncmp("eerstm1",smsbuffer,7)==0){//remote B
		ret_value = 1;
		//check access
		//if (sys_data.cli_access_level<2){ creatSMS("Unauthorized action",3); return; }
		Serial.println(F("EEPROM Reset"));
		eeprom_reset();
		
	}
	
	else if (strncmp("infor=",smsbuffer,6)==0)//request infor
	{
		
		
	}	
	else if (strncmp_P(smsbuffer,PSTR("Entry delay="),12)==0)
	{
		ret_value = 1;
		//check access
		if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3,0); }
		
		char zone_index[10];
		
		strlcpy(zone_index,smsbuffer+12,3);
		int entry_delay_int = atoi(zone_index);
		systemConfig.entry_delay_time = entry_delay_int;
		Serial.print("Entry delay:");
		Serial.println(entry_delay_int);
		myAlarm_pannel.set_entry_delay_timer_interval(entry_delay_int);
		eeprom_save();
		if (Invoker==GSM_MODULE)
		{
			char buffer[5];
			strcpy_P(buffer,PSTR("OK"));
			creatSMS(buffer,2,0);
		}
	}
	else if (strncmp(smsbuffer,PSTR("Exit delay="),11)==0)
	{
		ret_value = 1;
		//check access
		if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3,0); }
		
		char zone_index[10];
		strlcpy(zone_index,smsbuffer+11,3);
		int exit_delay_int = atoi(zone_index);
		systemConfig.exit_delay_time = exit_delay_int;
		Serial.print(F("Exit delay:"));
		Serial.println(exit_delay_int);
		myAlarm_pannel.set_exit_delay_timer_interval(exit_delay_int);
		eeprom_save();
		if (Invoker==GSM_MODULE)
		{
			char buffer[5];
			strcpy_P(buffer,PSTR("OK"));
			creatSMS(buffer,2,0);
		}
	}
	
	else if (strncmp("sc=",smsbuffer,3)==0)// sensor state update sc=02,1 : zone 2 opend
	{
		char zone_index[10];
		char zone_name[10];
		strlcpy(zone_index,smsbuffer+3,3);
		strlcpy(zone_name,smsbuffer+6,2);
		int zone = atoi(zone_index);
		int state = atoi(zone_name);
		myAlarm_pannel.Universal_zone_state_update(zone,state);
		//send update to MQTT
#ifdef MQTT_OK
		send_sensor_state_update_to_mqtt(zone,state);		
#endif		
	}

	else if (strncmp("Relay a on=",smsbuffer,10)==0)
	{
		ret_value = 1;
		//check access
		if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3,0); }
		//Relay_A.Activate(2000);
		char buffer[5];
		strcpy_P(buffer,PSTR("OK"));
		creatSMS(buffer,2,0);
	}
	else if (strncmp("Relay b on=",smsbuffer,10)==0)
	{
		ret_value = 1;
		//check access
		if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3,0); }
		//relay_B_activate();
		char buffer[5];
		strcpy_P(buffer,PSTR("OK"));
		creatSMS(buffer,2,0);
	}
	else if (strncmp("Relay b off=",smsbuffer,10)==0)
	{
		ret_value = 1;
		//check access
		if (systemConfig.cli_access_level<2){ creatSMS("Unauthorized action",3,0); }
		//relay_B_deactivate();
		creatSMS("Successful",2,0);
	}
	else if (strncmp("Power?",smsbuffer,6)==0)
	{
		ret_value = 1;
		creat_power_sms(systemConfig.ac_power);
		//check access
		// 		if (sys_data.cli_access_level<2){ creatSMS("Unauthorized action",3); return; }
		// 		relay_B_deactivate();
	}
	else if (strncmp("chime1",smsbuffer,6)==0)
	{
		ret_value = 1;
		xEventGroupSetBits(EventRTOS_buzzer,    TASK_3_BIT );
		
	}
	else if (strncmp("siren=",smsbuffer,6)==0)
	{
		ret_value = 1;
		char zone_index[10];
		strlcpy(zone_index,smsbuffer+6,2);
		
		uint8_t state = atoi(zone_index);
	
		if(state){xEventGroupSetBits(EventRTOS_buzzer,    TASK_2_BIT );}
		else{
			xEventGroupSetBits(EventRTOS_buzzer,    TASK_1_BIT );
		}
		
		
	}
	return ret_value;
}


void invoker_rec(){
	int index_result = inputString.indexOf(">");

	if (index_result!=-1){
		char invorker_id_char[2] = "";
		strlcpy(invorker_id_char,inputString.c_str(),2);
		uint8_t invoker_id = atoi(invorker_id_char);	
		//remove invoker header;
		inputString.remove(0,2);	
		
		switch (invoker_id)

		{

		case CARDA_ID:last_invorker = CARD_A;

			break;

		case CARDB_ID:last_invorker = CARD_B;

		break;

		case CARDC_ID:last_invorker = CARD_C;

		break;

		case GSM_MODULE_ID:last_invorker = GSM_MODULE;

		break;

		case LCDA_ID:last_invorker = LCD;

		break;
		
		case WEB:last_invorker = WEB;

		break;

		case APP:last_invorker = APP;

		break;
		}
			/*Serial.print("detected invoker ID:");
			Serial.print(invoker_id);
			Serial.print("converted typeDef:");
			Serial.println(last_invorker);*/
	}
	else{
		last_invorker  = PSERIAL0;
	}
}

int8_t remcode(const char* codeStr, int8_t *remoteUserID) {
    // Convert the string to an unsigned long integer
    unsigned long code = strtoul(codeStr, NULL, 10);
    // Determine if it's an old or new remote command based on command value
    uint8_t split_4bit = code & 0x0F; // Extract the last 4 bits
	uint8_t split_8bit = code & 0xFF; // Extract the last 8 bits
	uint8_t command=-1;
    uint32_t remoteID;

    // Check if it's an new remote command by comparing to known new commands
    if (split_4bit == 0x01 || split_4bit == 0x02 || split_4bit == 0x04 || split_4bit == 0x08) {
        // new remote command (4 bits)
        command = code & 0x0F; // Extract the last 4 bits
        remoteID = code >> 4;  // Shift right by 4 bits to get the remote ID
		*remoteUserID = comp_remote_RFID(remoteID, 4);
    } 

	else if(split_8bit == 0xC0 || split_8bit == 0x03 || split_8bit == 0x0C || split_8bit == 0x30){
        // New remote command (8 bits)
        command = code & 0xFF; // Extract the last 8 bits
        remoteID = code >> 8;  // Shift right by 8 bits to get the remote ID
		*remoteUserID = comp_remote_RFID(remoteID, 8);
    }

    // Print the results   
    switch (command) {
        case 0x01: // 0001 (new remote)
        case 0xC0: // 00000011 (old remote)            
			return 1;
            break;
        case 0x02: // 0010 (new remote)
        case 0x03: // 00001100 (old remote)            
			return 2;
            break;
        case 0x04: // 0100 (new remote)
        case 0x0C: // 00110000 (old remote)            
			return 3;
            break;
        case 0x08: // 1000 (new remote)
        case 0x30: // 11000000 (old remote)            
			return 4;
            break;
        default:
            Serial.println(F("Unknown Command"));
			return command;
            break;
    }
}

void RFbaster(const char* rf_id_msg){
if (strncmp("RID",rf_id_msg,3)==0)// rf id received  RFD=254266
	{				
		if (strncmp("=",rf_id_msg+3,1)==0)
		{			
			
			// check remote actions
			int8_t remote_user_index;
			int8_t remote_command = remcode(rf_id_msg+5, &remote_user_index);
			
			if ((remote_command!=-1)&&(remote_user_index != -1))
			{
				Serial.printf(("Remote command : %d\n"),remote_command);
				if(remote_command == 1)
				{
					Serial.println(F("key Arm"));
					myAlarm_pannel.set_arm_mode(AS_ITIS_NO_BYPASS);
					myAlarm_pannel.set_system_state(SYS1_IDEAL,RF,remote_user_index);
#ifdef MQTT_OK
					publish_system_state("ARMED","info/mode",true);
#endif
				}
				//if (strncmp("11",received_rf_id_char+18,2)==0)
				if(remote_command == 4)
				{
					Serial.println(F("key Panic"));
					myAlarm_pannel.set_system_state(ALARM_CALLING,RF,remote_user_index);
					char buffer[10];
					get_eInvoker_type_to_char(RF,buffer);
					creat_panic_sms(buffer);
					char zone_char[25];
					memset(zone_char,'\0',25);
					sprintf(zone_char,"Panic %s",buffer);
					const TickType_t x100ms = pdMS_TO_TICKS( 10 );
					/* Send the string to the message buffer.  Return immediately if there is
					not enough space in the buffer. */
					size_t xBytesSent;
					xBytesSent = xMessageBufferSend( xMessageBuffer_zone,
												( void * ) zone_char,
												strlen( zone_char ), 0 );

					if( xBytesSent != strlen( zone_char ) )
					{
						Serial.println(F("The string could not be added to the message buffer because there was not enough free space in the buffer"));
						
					}
					
				}
				if (remote_command==3)
				{
					Serial.println(F("key Away"));
				}
				//if (strncmp("11",received_rf_id_char+22,2)==0)
				if(remote_command == 2)
				{
					Serial.println(F("key Disarm"));
					myAlarm_pannel.set_system_state(DEACTIVE,RF,remote_user_index);
					eCurrent_state =  DEACTIVE;
#ifdef MQTT_OK
					publish_system_state("DISARMED","info/mode",true);
#endif                    
				}
			}
			else{
				Serial.println(F("Not a Remote"));
				// get RFID and pass it to the function
			char RFID_char[15]={0};
			strlcpy(RFID_char,rf_id_msg+5,9);	
			int8_t zone = comp_device_RFID(RFID_char);
			send_rfid_state_update_to_mqtt(RFID_char);
			if(zone!=-1){ 
				myAlarm_pannel.Universal_zone_state_update(zone, 1); 
#ifdef MQTT_OK
				send_sensor_state_update_to_mqtt(zone,1);		
#endif						
			}
			}

					
		}
		
	}					
}
	
	
void RFListiner(){
		if (mySwitch.available()){
			char buff[10]="";
			unsigned long now_RFID = mySwitch.getReceivedValue();
			Serial.println(now_RFID);
			if (now_RFID!= prev_RFID)
			{prev_RFID = now_RFID;
				rf_id_automatic_clr_timer_en= true;
				Timer_rf_id_auto_clr.interval=3000;
				Timer_rf_id_auto_clr.previousMillis=millis();
				
				rfid_int_to_str(buff,now_RFID);
				//filter some noise
				if (strlen(buff)<4)
				{
					return;
				}
				xEventGroupSetBits(EventRTOS_buzzer,    TASK_7_BIT );// RF rx buzzer event
					char zone_name[15]="";
					sprintf(zone_name,"RID=,%s",buff);
					transfer_rf_scan_data(zone_name);
					RFbaster(zone_name);
				
			}
			else{
				mySwitch.resetAvailable();
			}
			
			
		}
		if (rf_id_automatic_clr_timer_en)
		{
			if (Timer_rf_id_auto_clr.Timer_run())
			{
				rf_id_automatic_clr_timer_en = false;
				
				prev_RFID = 0;
			}
		}
	}
	
	void rfid_int_to_str(char* buff, unsigned long rf_id){
		
		mySwitch.resetAvailable();
		ultoa	(rf_id, buff,10);
	}
	
	
	void get_eInvoker_type_to_char(eInvoking_source invoker, char* buffer){
		
		switch (invoker)
		{
			case PSERIAL0:strcpy_P(buffer,PSTR("USART"));
			break;
			case LCD: strcpy_P(buffer,PSTR("LCD"));
			break;
			case CARD_A: strcpy_P(buffer,PSTR("CARD  A"));
			break;
			case CARD_B: strcpy_P(buffer,PSTR("CARD B"));
			break;
			case CARD_C: strcpy_P(buffer,PSTR("CARD C"));
			break;
			case GSM_MODULE: strcpy_P(buffer,PSTR("GSM"));
			break;
			case RF: strcpy_P(buffer,PSTR("REMOTE"));
			break;
			case SYSTEM_ITSELF: strcpy_P(buffer,PSTR("SYSTEM"));
			break;
			case INTERNEL_KEY_PAD: strcpy_P(buffer,PSTR("KEY PAD"));
			break;
			case WEB:strcpy_P(buffer,PSTR("WEB"));
			break;
			case APP:strcpy_P(buffer,PSTR("APP"));
			break;
			default: strcpy_P(buffer,PSTR("UNKNOWN"));
			break;
		}
		
		
	}
	
	
	void disarm_busy_buzzer(){
		for (uint8_t n=0;n<4;n++)
		{
			digitalWrite(BuzzerPin,HIGH);
			delay(100);
			digitalWrite(BuzzerPin,LOW);
			delay(50);
		}
	}
	void arm_busy_buzzer(){
		for (uint8_t n=0;n<2;n++)
		{
			digitalWrite(BuzzerPin,HIGH);
			delay(100);
			digitalWrite(BuzzerPin,LOW);
			delay(50);
		}
	}
	
	
	void lcd_write_line(char *buffer, uint8_t line){
		while(!(xSemaphoreTake( xMutex_I2C, portMAX_DELAY )));
		lcd.setCursor(0,line);
		xSemaphoreGive(xMutex_I2C);
		uint8_t len = strlen(buffer);
		for(uint8_t zone_index_int=0; zone_index_int<len; zone_index_int++){
			while(!(xSemaphoreTake( xMutex_I2C, portMAX_DELAY )));
			lcd.print(buffer[zone_index_int]);
			xSemaphoreGive(xMutex_I2C);
			delay(20);
		}
		if (len<16)
		{
			for(uint8_t zone_index_int=len; zone_index_int<16; zone_index_int++){
				while(!(xSemaphoreTake( xMutex_I2C, portMAX_DELAY )));
				lcd.print(" ");
				xSemaphoreGive(xMutex_I2C);
				
			}
		}
		
	}
	
	
void buzzer(){
	static eBuzzer_state eCurrent_buzzer_state, ePrev_buzzer_state;
		const TickType_t xTicksToWait = pdMS_TO_TICKS( 1);
		EventBits_t uxBits;
		uxBits = xEventGroupWaitBits(
		EventRTOS_buzzer,   /* The event group being tested. */
		ALL_SYNC_BITS, /* The bits within the event group to wait for. */
		pdTRUE,        /* BITS should be cleared before returning. */
		pdFALSE,       /* Don't wait for all bits, either bit will do. */
		xTicksToWait );/* Wait a maximum of 100ms for either bit to be set. */
		
		
		  if(  ( uxBits & TASK_1_BIT ) != 0  )// Buzzer off
		  {
			  
			  Serial.println(F("EVENT BUZZER_OFF"));
			  eCurrent_buzzer_state = BUZZER_OFF;
			  
		  }
		  if(  ( uxBits & TASK_2_BIT ) != 0  )// Disarm
		  {
			  
			   Serial.println(F("EVENT BUZZER_ALARM"));
			   eCurrent_buzzer_state = BUZZER_ALARM;
		  }
		   if(  ( uxBits & TASK_3_BIT ) != 0  )// Disarm
		   {
			   
			   Serial.println(F("EVENT BUZZER_RF"));
			  if(eCurrent_buzzer_state != BUZZER_ALARM){ 
					eCurrent_buzzer_state = BUZZER_RF;
					ePrev_buzzer_state = BUZZER_OFF;// bug fix for no buzzer sound when sensor trigger in disarm mode
				}
			   
			  
		   }
		    if(  ( uxBits & TASK_4_BIT ) != 0  )// Disarm
		    {
			    
			    Serial.println(F("EVENT BUZZER_GSM_ERROR"));
			    eCurrent_buzzer_state = BUZZER_GSM_ERROR;
		    }
			if(  ( uxBits & TASK_5_BIT ) != 0  )// Disarm
			{				
				Serial.println(F("EVENT BUZZER_ARM"));
				eCurrent_buzzer_state = BUZZER_ARM;
				arm_busy_buzzer();
			}
			if(  ( uxBits & TASK_6_BIT ) != 0  )// Disarm
			{
				
				Serial.println(F("EVENT BUZZER_DISARM"));
				eCurrent_buzzer_state = BUZZER_DISARM;
				disarm_busy_buzzer();
			}
			if(  ( uxBits & TASK_7_BIT ) != 0  )// Disarm
			{
				 digitalWrite(PIN_RF_LED,HIGH);
				 delay(50);
				 digitalWrite(PIN_RF_LED,LOW);
			}
		  
		  
		  
		switch (eCurrent_buzzer_state)
		{
		case BUZZER_OFF:{
			if (ePrev_buzzer_state!=eCurrent_buzzer_state)
			{
				ePrev_buzzer_state=eCurrent_buzzer_state;
				
			}
			digitalWrite(BuzzerPin,LOW);
		}
			break;
		case BUZZER_ALARM:{
			if (ePrev_buzzer_state!=eCurrent_buzzer_state)
			{
				ePrev_buzzer_state=eCurrent_buzzer_state;
			}
			digitalWrite(BuzzerPin,HIGH);
			delay(180);
			digitalWrite(BuzzerPin,LOW);
			delay(180);
		}
		break;
		case BUZZER_RF:{
			if (ePrev_buzzer_state!=eCurrent_buzzer_state)
			{
				ePrev_buzzer_state=eCurrent_buzzer_state;
				digitalWrite(BuzzerPin,HIGH);
				delay(180);
				digitalWrite(BuzzerPin,LOW);
				delay(180);
				eCurrent_buzzer_state = BUZZER_OFF;
			}
		}
		break;
		case BUZZER_GSM_ERROR:{
			if (ePrev_buzzer_state!=eCurrent_buzzer_state)
			{
				ePrev_buzzer_state=eCurrent_buzzer_state;
			}
		}
		break;
		}
	}
	
	

	
	
void Power_detect_loop() {
	static int powerState = digitalRead(PIN_AC_DETECT);
	static unsigned long stableStart = 0;
	static bool isStable = false;
	
	int newState = digitalRead(PIN_AC_DETECT);
	if (newState != powerState) {
		// power state has changed, reset stable state tracking
		isStable = false;
		// power state is not stable yet, reset stable duration timer
		stableStart = millis();
		Serial.println("power state has changed");
	}
	
	powerState = newState;
	
	if (!isStable) {	
	if (millis() - stableStart >= stableDuration) {
		// power state has been stable for long enough
		isStable = true;
		
		if (powerState == LOW) {
			Serial.println(F("Power on!"));
			systemConfig.ac_power = true;
			creat_power_sms(true);
			} else {
			Serial.println(F("Power off!"));
			creat_power_sms(false);
			systemConfig.ac_power=false;
		}
	}
	
	}
}


void send_all_zone_states_mqtt(){
	
            for (int i = 0; i < TOTAL_DEVICES; ++i) {
				if(myAlarm_pannel.is_sensor_available(i)){
					bool state = getSensor(i);
                	send_sensor_state_update_to_mqtt(i, state);
				}

				if(myAlarm_pannel.is_sensor_RF(i)){
					bool state = getSensor(i);
                	send_sensor_state_update_to_mqtt(i, state);
				}
                
                
            }
           
}
