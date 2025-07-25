
#include "gsm_broker.h"
char Module_IMEI_p[] PROGMEM = "868428042211700";//868428042211700
HardwareSerial *fonaSerial = &Serial2;
GSM_stateMachineStates eCurruntGSM_state = GSM_INIT, ePrevGSM_state = GSM_SMS_SUSPENDING;
bool sms_sending_queu_complete = false;
bool sms_broadcast_request = false;
uint8_t sms_broadcast_index=1;
uint8_t sms_buffer_msg_count=0;
char local_smsbuffer[SMS_BUFFER_LENGTH];
bool request_from_sim800 = false;
bool thisIs_Restart = true;
bool gsm_init_done = false;
bool gsm_available = false;
bool timesync_need = true;
uint8_t gsmsignal_rssi;
// Use this for FONA 800 and 808s
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

uint8_t alarm_calling_index=1;
bool sms_hdrl_suspend=false;

uint8_t Current_caller_state = 0;

uint8_t gsm_init(){
	
	 Serial.println(F("SIM800 Initializing....)"));
	 fonaSerial->begin(9600, SERIAL_8N1, ESP_RXD, ESP_TXD);
	 if (! fona.begin(*fonaSerial)) {
		 Serial.println(F("Couldn't find FONA"));
		 //while (1);
		 return NO_RESP;
	 }
#ifdef _DEBUG
	 uint8_t type = fona.type();
	 Serial.println(F("FONA is OK"));
	 Serial.print(F("Found "));
	 switch (type) {
		 case FONA800L:
		 Serial.println(F("FONA 800L")); break;
		 case FONA800H:
		 Serial.println(F("FONA 800H")); break;
		 case FONA808_V1:
		 Serial.println(F("FONA 808 (v1)")); break;
		 case FONA808_V2:
		 Serial.println(F("FONA 808 (v2)")); break;
		 case FONA3G_A:
		 Serial.println(F("FONA 3G (American)")); break;
		 case FONA3G_E:
		 Serial.println(F("FONA 3G (European)")); break;
		 default:
		 Serial.println(F("???")); break;
	 }
#endif	
	 // Print module IMEI number.
	 char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!
	 uint8_t imeiLen = fona.getIMEI(imei);
	 if (imeiLen > 0) {
		 Serial.print(F("Module IMEI: ")); Serial.println(imei);
	 }

	 fona.deleteAllSMS();
	 // Check SIM card presence
    if (!fona.isSIMInserted()) {
        Serial.println(F("NO SIM!"));
        return NO_SIM;
    }
    Serial.println(F("SIM READY."));	 
	 
#ifdef GSM_MODULE_AUTH
	 if (thisIs_Restart==true)
	 {
		if(strcmp_P(imei,Module_IMEI_p)==0){
			Serial.println(F("Auth OK"));
		}
		else{
			Serial.print(imei);
			Serial.println(F(" Auth Fail"));
			delay(500);
			ESP.restart();
			return AUTH_FAILD;
		}		
		
	 }	 
	 thisIs_Restart = false;
#endif	 
		return SIM800_OK;
	 
}


uint8_t getSignal_strength(){

	return  gsmsignal_rssi;
}

bool setTime_from_gsm(){
	char networkTime[100];
	// char localTime[20];
	if (fona.getTime(networkTime, 100)) {
		 setESP32_rtc(networkTime);
		 return true;
	 }
	Serial.println(F("Failed to get time from GSM module"));
	return false;
}

void creatSMS(const char* buffer,uint8_t type, const char* number){// creat a SMS
	int msg_length = strlen(buffer);
	char sms_159[159]="";
	if (msg_length>155)
	{
		Serial.println(F("msg is beyond the size of sms. so Split it"));
		strlcpy(sms_159,buffer,155);
		//creatSMS_LL(sms_159,type,0),number;
		addSMS(sms_159,type,number);
		addSMS(buffer+155,type,number);
		
	}
	else{
		addSMS(buffer,type, number);
	}
	DynamicJsonDocument doc(2048);
	doc["msgTyp"] = type;
  	doc["msg"] = buffer;
	saveMessageToSPIFFSV3(doc);
	
}

// void creatSMS_LL(const char* buffer,uint8_t type,const char* number){
	
// 	Serial.print(F("creating SMS>"));
// 	// Serial.println(buffer);
// 	// sms_buffer_msg_count = sms_buffer_msg_count + 1;
// 	// delay(1);
	

//     addSMS(buffer,type,number);  // Pass safe, independent buffer to addSMS

// 	sms_broadcast_request = true;	
// }


void ultimate_sms_hadlr() {

    static SMS_t sms;                // Holds the current SMS message being processed
    static int state = 0;            // Tracks current state of the SMS state machine
    static int contact_index = 1;    // Keeps track of which contact to send next (for broadcasts)
    static char num[20];             // Temporary buffer to hold target phone numbers

    switch (state) {

        case 0: {  // IDLE State - Check for pending SMS in queue or suspend conditions

            bool should_I_be_suspend = false; // Flag to decide if task should suspend
            bool is_suspend_request = check_SMS_TASK_suspend_request();  // Check for external suspend signal

            // Check if any SMS messages are pending in the queue
            if (uxQueueMessagesWaiting(smsQueue) > 0) {
                if (getNextSMS(&sms)) {  // Fetch next SMS from queue
                    state = 1;  // Move to next state to determine where to send
                }
            }
			else{
				Serial.println(F("SMS TASK SUSPENDED IT SELF AS NO NUMBER"));
                should_I_be_suspend = true;
                sms_sending_queu_complete = true;  // Reset your own flag

			}

            // If external suspend requested, prepare to suspend task
            if (is_suspend_request) {
                Serial.println(F("AS REQUESTED SMS TASK SUSPENDING..."));
                should_I_be_suspend = true;
            }

            // If suspension is required, signal and suspend this task
            if (should_I_be_suspend) {
                xEventGroupSetBits(EventRTOS_gsm, TASK_4_BIT);  // Notify system task is suspending
                vTaskSuspend(NULL);  // Suspend the current task until resumed externally
            }

        }
        break;

        case 1: {  // Determine where to send the SMS based on its type

            if (sms.type == 4) {
                // Type 4: Send to a specific number provided in the SMS struct
                strncpy(num, sms.number, sizeof(num) - 1);
                num[sizeof(num) - 1] = '\0';  // Null-terminate to avoid memory issues

                if (phone_number_validat(num)) {
                    state = 2;  // Valid number, move to sending stage
                } else {
                    state = 0;  // Invalid number, discard and return to idle
                }

            } else if (sms.type == 3) {
                // Type 3: Send to the last known SMS sender (stored globally)
                strncpy(num, systemConfig.last_sms_sender, sizeof(num) - 1);
                num[sizeof(num) - 1] = '\0';

                if (phone_number_validat(num)) {
                    state = 2;  // Valid, move to sending stage
                } else {
                    state = 0;  // Invalid, discard and go idle
                }

            } else {
                // Type 1 or other: Broadcast to the predefined contact list
                contact_index = 1;  // Start with first contact
                state = 3;  // Move to broadcast state
            }

        }
        break;

        case 2: {  // Send SMS to a single specific number (Type 3 or 4)

            waitForMutex_GSM();  // Lock GSM module to ensure safe communication

            if (!fona.sendSMS(num, sms.message)) {
                // Sending failed, release GSM and retry same number next cycle
                releaseMutex_GSM();
                state = 2;
            } else {
                // SMS sent successfully, release GSM and return to idle
                releaseMutex_GSM();
                state = 0;
            }

        }
        break;

        case 3: {  // Broadcast SMS to multiple numbers in the contact list

            // Get the next contact number from your contact list
            strncpy(num, get_GSM_number(contact_index), sizeof(num) - 1);
            num[sizeof(num) - 1] = '\0';
			if( strstr(num, "N") != NULL) {
				// If the number is flagged as 'N', skip to next contact
				state = 0;  // All contacts done, return to idle
			}

            // Validate the number, skip if invalid or flagged as 'N' (not a valid contact)
            if (!phone_number_validat(num) || !get_is_GSM_number_sms(contact_index)) {
                contact_index++;  // Skip to next contact
				Serial.print(F("Skipping invalid or non-SMS contact: "));
				Serial.println(num);

                if (contact_index >= 8) {
                    state = 0;  // End of contact list reached, go idle
                }
            } else {
                waitForMutex_GSM();  // Lock GSM module for safe sending

                if (!fona.sendSMS(num, sms.message)) {
                    // Failed to send, release GSM and retry this number next time
                    releaseMutex_GSM();
                } else {
                    // Successfully sent, release GSM and move to next contact
                    releaseMutex_GSM();
                    contact_index++;

                    if( (contact_index >= 8) || strstr(num, "N"))  {
                        state = 0;  // All contacts done, return to idle
                    }
                }
            }

        }
        break;
    }
}




bool check_SMS_TASK_suspend_request(){
	bool ret= false;
	/* Wait a maximum of 100ms for either bit 0 or bit 4 to be set within
		  the event group.  Clear the bits before exiting. */
		const TickType_t xTicksToWait = 100 / portTICK_PERIOD_MS;
		EventBits_t uxBits;
		uxBits = xEventGroupWaitBits(
					EventRTOS_gsm,   /* The event group being tested. */
					TASK_3_BIT, /* The bits within the event group to wait for. */
					pdTRUE,        /* BIT_1 should be cleared before returning. */
					pdFALSE,       /*wait for all both bits, . */
					xTicksToWait );/* Wait a maximum of 100ms for either bit to be set. */

		 if(  ( uxBits & TASK_3_BIT ) != 0  )// SMS TASK SUSPEND request
		 {
			
			 xEventGroupClearBits(EventRTOS_gsm,    TASK_3_BIT );
			ret = true;
			 
		 }
  
	return ret;
}
			

boolean phone_number_validat(const char* num_char){
	
	if (num_char[0] == '+') {
			if (strlen(num_char) < 12) {
				Serial.print(F("Length Invalid phone number.\n"));
				return false;
			}
		} else if (num_char[0] == '0') {
			if (strlen(num_char) != 10) {
				Serial.print(F("Length Invalid phone number.\n"));
				return false;
			}
		} else {
		Serial.print(F("Invalid phone number.\n"));
		return false;
	}

	for (int i = 1; i < strlen(num_char); i++) {
		if (num_char[i] < '0' || num_char[i] > '9') {
			Serial.println(F("Char Invalid phone number.\n"));
			return false;
		}
	}

	//Serial.println(F("Valid phone number"));
	return true;
	
}

boolean phone_number_correction(char *num_char){

	if (num_char[0] == '+') {
		if (strlen(num_char) != 12 || strncmp(num_char+1, "94", 2) != 0) {
			Serial.print(F("Invalid telephone number.\n"));
			return false;
		}
		} else if (num_char[0] == '0') {
		char temp[20];
		strcpy(temp, "+94");
		strcat(temp, num_char+1);
		if (strlen(temp) != 12) {
			Serial.print(F("Invalid telephone number.\n"));
			return false;
		}
		strcpy(num_char, temp);
		} else {
		Serial.print(F("Invalid telephone number.\n"));
		return false;
	}

	Serial.print(F("Valid telephone number: %s\n"));
	return true;
}


void flushSerial() {
	while (Serial.available())
	Serial.read();
}

bool ultimate_gsm_listiner(){
	uint8_t  position ;
	 waitForMutex_GSM();	
	
	  if (fona.getCallStatus()==3)
	  {
		  Serial.println(F("there is incoming call I discarded it"));
		  fona.hangUp();
	  }	
	  delay(100);  
	    uint16_t smslen;
		char n[20]="";		
		// read new SMS
	    memset(local_smsbuffer,'\0',160);		 
		//Serial.print(F("Reading New SMS>"));
		position = fona.getSMSIndex(SMS_ALL);
		if (position>0)// we have valid sms to read out
		{
			  // incoming_sms_process(sms_read_fn_result);
			  // Retrieve SMS value.
			  fona.GetSMSx(position, n, 20,local_smsbuffer,SMS_BUFFER_LENGTH,&smslen);
			  // now we have phone number string in phone_num
			  // and SMS text in sms_text
#ifdef _DEBUG
			  Serial.println(n);
			  Serial.printf_P(PSTR("SMS>%s"),local_smsbuffer);
#endif	  
			  fona.deleteSMS(position);
			  releaseMutex_GSM();
			  incoming_sms_process(local_smsbuffer, n);
			  position = 0;			  
		  }
		  else
		  {
			  //Serial.println(F("NO SMS FOUND"));
			  releaseMutex_GSM();
		  }

		  return true;
}

// Check network registration
// 0: Not registered, MT is not currently searching a new operator to
//  register to
// 1: Registered, home network
// 2: Not registered, but MT is currently searching a newoperator to register
//  to
// 3: Registration denied
// 4: Unknown
// 5: Registered, roaming
void gsm_manager(){
	//static uint8_t old_network_status = 0;
	const TickType_t xTicksToWait = 100 / portTICK_PERIOD_MS;
	EventBits_t uxBits;
	uxBits = xEventGroupWaitBits(
	EventRTOS_gsm,   /* The event group being tested. */
	ALL_SYNC_BITS, /* The bits within the event group to wait for. */
	pdFALSE,        /* BITS should NOT be cleared before returning. */
	pdFALSE,       /* Don't wait for all bits, either bit will do. */
	xTicksToWait );/* Wait a maximum of 100ms for either bit to be set. */
	
	switch (eCurruntGSM_state)
	{
		case GSM_INIT:{
			if (ePrevGSM_state!=eCurruntGSM_state)
			{
				ePrevGSM_state = eCurruntGSM_state;
				Serial.println(F("GSM_INIT"));
				//set gsmled to off event
				xEventGroupSetBits(EventRTOS_gsmled,    TASK_5_BIT );// gsm led init				
  				//uint32_t red = Adafruit_NeoPixel::Color(255, 0, 0);
  				//pixel.startBlink(red, 300, 300, 180);
			}
			waitForMutex_GSM();
			uint8_t gsm_init_status = gsm_init();
			releaseMutex_GSM();
			if(gsm_init_status==SIM800_OK){

				eCurruntGSM_state = GSM_LISTIN;
				Serial.println(F("GSM initializing OK"));
				xEventGroupSetBits(EventRTOS_gsmled,    TASK_5_BIT );// gsm led init
			}
			else if(gsm_init_status==NO_SIM){
				Serial.println(F("GSM NO SIM"));
				eCurruntGSM_state = GSM_INIT;
				xEventGroupSetBits(EventRTOS_gsmled,    TASK_1_BIT );// gsm no sim led off
				return; // Wait for network to be ready
			}
			else if(gsm_init_status == NO_RESP){
				Serial.println(F("GSM NO RESP"));
				eCurruntGSM_state = GSM_INIT;
				xEventGroupSetBits(EventRTOS_gsmled,    TASK_1_BIT );// gsm no resp led off
				return; // Wait for network to be ready
			}
			else if(gsm_init_status == AUTH_FAILD){
				Serial.println(F("GSM AUTH FAILD"));
				eCurruntGSM_state = GSM_INIT;
				xEventGroupSetBits(EventRTOS_gsmled,    TASK_1_BIT );// gsm no auth led off
				return; // Wait for network to be ready
			}
			

		} break;
		case GSM_LISTIN:{
			if (ePrevGSM_state!=eCurruntGSM_state)
			{
				ePrevGSM_state = eCurruntGSM_state;
				Serial.println(F("GSM_LISTIN"));
			}
			// Check network registration
				waitForMutex_GSM();
				uint8_t network_status = fona.getNetworkStatus();
				releaseMutex_GSM();
				if (network_status == 0) {
					Serial.println(F("Network Status: Not registered"));
					
				} else if (network_status == 2) {
					Serial.println(F("Network Status: Searching for network"));
					xEventGroupSetBits(EventRTOS_gsmled,    TASK_2_BIT );// gsm no network
			 		return; // Wait for network to be ready
					
				} else if (network_status == 3) {
					Serial.println(F("Network Status: Registration denied"));
					eCurruntGSM_state = GSM_INIT;
					xEventGroupSetBits(EventRTOS_gsmled,    TASK_3_BIT );// gsm no network
					return; // Wait for network to be ready
					
				} else if (network_status == 4) {
					Serial.println(F("Network Status: Unknown"));
					eCurruntGSM_state = GSM_INIT;
					xEventGroupSetBits(EventRTOS_gsmled,    TASK_3_BIT );// gsm no network
					return; // Wait for network to be ready
					
				} else if( network_status == 5) {
					Serial.println(F("Network Status: Roming Network"));
					eCurruntGSM_state = GSM_INIT;
			 		xEventGroupSetBits(EventRTOS_gsmled,    TASK_6_BIT );// gsm no network
					
				}else if (network_status == 1) {
					Serial.println(F("Network Status: Registered, home network"));
					// Print network status
				Serial.print(F("Network Status: ")); Serial.println(network_status);
				if(gsm_init_done == false){
					// Set SMS text mode and storage
					waitForMutex_GSM();
					if(!fona.setSMSTextModeAndStorage("ME")) {
						Serial.println(F("Failed to set SMS text mode and storage"));
					} else {
						Serial.println(F("SMS text mode and storage set to ME"));
						gsm_init_done = true;
					}					
					releaseMutex_GSM();					
				}
				if(timesync_need){
					releaseMutex_GSM();
					if(setTime_from_gsm()){
						Serial.println(F("Time set from GSM"));
						timesync_need = false;

					}else{
						Serial.println(F("Failed to set time from GSM"));
					}
					releaseMutex_GSM();
				}
					

				// Check signal strength
				waitForMutex_GSM();
				gsmsignal_rssi = fona.getRSSI();
				releaseMutex_GSM();
				Serial.print(F("Signal strength: ")); Serial.print(gsmsignal_rssi); Serial.println(F(" dBm"));
				if(gsmsignal_rssi < 10){
					Serial.println(F("GSM Signal Low"));
			 		xEventGroupSetBits(EventRTOS_gsmled,    TASK_4_BIT );// gsm no network
				}
				else{
					Serial.println(F("GSM Network OK"));
			 		xEventGroupSetBits(EventRTOS_gsmled,    TASK_6_BIT );// gsm network ok

				}
				}
			// Check if there is an incoming SMS
			
			bool gsm_state =  ultimate_gsm_listiner();
			if(!gsm_state){ }// network erro count to reset gsm module

			// Declare a variable to hold the number of messages waiting in the queue
			UBaseType_t queueLength = uxQueueMessagesWaiting(smsQueue);

			// Check if there is at least one message in the queue
			if (queueLength > 0) {
				// There is at least one message in the queue
#ifdef _DEBUG
				printf("There are %d sms in the queue.\n", queueLength);
				Serial.println(F("sms task invoike"));
#endif
				//sms_broadcast_request = false;
				sms_sending_queu_complete = true;
				EventBits_t uxBits;
				uxBits = xEventGroupSetBits(EventRTOS_gsm,    TASK_5_BIT );// SMS task invoking request
			} else {
				// The queue is empty
#ifdef _DEBUG
				printf("The queue is empty.\n");
#endif
			}
			 if(sms_broadcast_request == true){
				// Serial.println(F("sms task invoike"));
				// sms_broadcast_request = false;
				// sms_sending_queu_complete = true;
				// EventBits_t uxBits;
				// uxBits = xEventGroupSetBits(EventRTOS_gsm,    TASK_5_BIT );// SMS task invoking request
			 }
			 if(  ( uxBits & TASK_1_BIT ) != 0  )// CALL TASK invoking request form alarm notify functions.
			 {
				 /* Clear bit 1. */
				 uxBits = xEventGroupClearBits(
				 EventRTOS_gsm,  /* The event group being updated. */
				 TASK_1_BIT);/* The bits being cleared. */
				 //suspend req for SMS TASK
				// EventBits_t uxBits;
				 uxBits = xEventGroupSetBits(EventRTOS_gsm,    TASK_3_BIT );// sms task suspend request sent
				 eCurruntGSM_state = GSM_SMS_SUSPENDING;
			 }
			 
			else if(  ( uxBits & TASK_5_BIT ) != 0  )// SMS resume.
			{				
				Serial.println(F("SMS TASK resume"));
				uxBits = xEventGroupClearBits(
				EventRTOS_gsm,  /* The event group being updated. */
				TASK_5_BIT);/* The bits being cleared. */
				
					/* Clear bit 0 and bit 4 in xEventGroup. */
					uxBits = xEventGroupClearBits(
					EventRTOS_gsm,  /* The event group being updated. */
					TASK_4_BIT);/* The bits being cleared. */
					
					/* Clear bit 0 and bit 4 in xEventGroup. */
					uxBits = xEventGroupClearBits(
					EventRTOS_gsm,  /* The event group being updated. */
					TASK_3_BIT);/* The bits being cleared. */
					
				vTaskResume(Task2_sms);//resume SMS TASK				
			}
		}
		break;
		
		case GSM_SMS_SUSPENDING:{
			if (ePrevGSM_state!=eCurruntGSM_state)
			{
				ePrevGSM_state = eCurruntGSM_state;
				Serial.println(F("GSM_SMS_SUSPENDING..."));
			}
			if(  ( uxBits & TASK_4_BIT ) != 0  )// SMS TASK suspended ACK recevide.
			{
				Serial.println(F("GSM_SMS_TASK_SUSPENDED."));			
				eCurruntGSM_state = GSM_CALL;				
				
			}
		}
		break;
		
		case GSM_CALL:{
			if (ePrevGSM_state!=eCurruntGSM_state)
			{
				ePrevGSM_state = eCurruntGSM_state;
				Serial.println(F("GSM_CALL"));
				//EventBits_t uxBits;
				uxBits = xEventGroupSetBits(EventRTOS_gsm,    TASK_6_BIT );// Call in progress bit
				uxBits = xEventGroupClearBits(EventRTOS_gsm,TASK_7_BIT );// call task invoking request
				delay(500);
				xTaskCreatePinnedToCore(Task5code_call,"Task5",5000,NULL,3,&Task5,0);
			}
			Serial.println(F("wait for call task resp"));
			if(  ( uxBits & TASK_2_BIT ) != 0  )// This bit will be set by Disarm call back so request to delete call task
			{
				Serial.println(F("CALL TASK delete request rent"));
				xEventGroupClearBits(EventRTOS_gsm,    TASK_2_BIT );// clear event
	
				//EventBits_t uxBits;
				uxBits = xEventGroupSetBits(EventRTOS_gsm,TASK_7_BIT );// call task delete request
				eCurruntGSM_state = GSM_CALL_TASK_DELETEING;	
			}
			
			if(  ( uxBits & TASK_6_BIT ) != 0  )// wait for call task complete 
			{
				
			}
			else{
				eCurruntGSM_state = GSM_LISTIN;
				
			}
			
		}
		break;
		case GSM_CALL_TASK_DELETEING:{
			if (ePrevGSM_state!=eCurruntGSM_state)
			{
				ePrevGSM_state = eCurruntGSM_state;
				Serial.println(F("GSM_CALL_TASK_DELETING"));
				
			}
			if(  ( uxBits & TASK_6_BIT ) != 0  )// Disarm Event task delete
			{
			}
			else{
				eCurruntGSM_state = GSM_LISTIN;
			}
		}
		break;
	}
	
}


void incoming_sms_process(char* local_smsbuffer, char* n ){
	 
		publish_incomming_sms_to_mqtt(local_smsbuffer, n );
		 //validate number
		 if (!phone_number_validat(n))// validating number
		 {
			 Serial.println(F("Validating phone number Error! do not reply"));
			 return;
		 }
		 strlcpy(systemConfig.last_sms_sender,n,13);
		 //check security level
		 Serial.print(F("check security level:"));
		 int8_t user_id = comp_User_id(n);
		 if ((user_id==-1)&&(strncmp("+94714427691",n,12)==0))
		 {
			 user_id = 255;
		 }
		 else if((user_id!=-1)){
			 // security bypass for +94714427691
			 if (strncmp("+94714427691",n,12)==0)
			 {
				  systemConfig.cli_access_level = 3;
			 }
			 
			 else{
				  systemConfig.cli_access_level = get_GSM_number_security_level(user_id);
			 }
#ifndef GSM_PULSEX_IOT_BOARD
			 if (!digitalRead(PROGRAM_PIN))// give full access if the program pin is low
			 {
				  systemConfig.cli_access_level = 3;
			 }
#endif			 
			 
			 byte cli_result = universal_event_hadler(local_smsbuffer,GSM_MODULE,user_id);// run CLI
			 if (!cli_result)
			 {
				 Serial.println(F("unknown command"));
				 creatSMS("Unknown command!",3,0);// only for programmer
			 }
			 
		 }
		 else{Serial.println(F("Command can't be accepted.Unauthorized number!"));
			// strlen_P(MSG19_CAN_NOT_ACCEPT);
			char buffer[50];
			memset(buffer,'\0',50);
			 strcat_P(buffer,PSTR("Unauthorized number!"));
			 creatSMS(buffer,3,0);
		 }
		 
		 request_from_sim800=false;
		 
	 
}



void creat_arm_sms(char* str_invorker){
	
	char sms_buffer[160];
	memset(sms_buffer,'\0',160);
	strcat_P(sms_buffer,PSTR("Home arm by"));
	strcat(sms_buffer,str_invorker);
	bool open_zone_yes=false;
	char msg_line_number_char[5]="";
	char open_zone_msg[100]="";
	int msg_line_number_int=0;
	for (int index=0; index<TOTAL_DEVICES; index++)
	{
		
		if ((!myAlarm_pannel.is_sensor_ready(index))&&(myAlarm_pannel.is_sensor_available(index)))
		{
			open_zone_yes=true;
			itoa(msg_line_number_int,msg_line_number_char,10);
			strcat(open_zone_msg,"*");
			//strcat(open_zone_msg,".");
			
			strcat(open_zone_msg,get_device_name(index));
			strcat(open_zone_msg,"\n");
			msg_line_number_int++;
			if (msg_line_number_int>8)
			{
				memset(open_zone_msg,'\0',100);
				strcat(open_zone_msg,"Too many faulty zones");
				break;
			}
		}
		
	}
	if(open_zone_yes){
		strcat(sms_buffer,"\n");
		strcat_P(sms_buffer,PSTR("OPEN ZONES"));
		strcat(sms_buffer,"\n");
	}
	uint8_t msg_length = strlen(open_zone_msg);
	if (msg_length<160)
	{
		strcat(sms_buffer,open_zone_msg);
		//add time stamp
		addTimeStamp(sms_buffer);
		creatSMS(sms_buffer,1,0);
	}
	else{
		//add time stamp
		addTimeStamp(sms_buffer);
		creatSMS("Too big msg",1,0);		
	}	
}

void creat_disarm_sms(char* str){
	char buff[100];
	memset(buff,'\0',100);
	strcpy_P(buff,PSTR("Disarm by "));
	strcat(buff,str);// where i left the code*********************************************code stuck here
	//add time stamp
	addTimeStamp(buff);
	creatSMS(buff,1,0);
}

void creat_panic_sms(char* str){
	char buff[100];
	memset(buff,'\0',100);
	strcpy_P(buff,PSTR("Panic Alarm by "));
	strcat(buff,str);// where i left the code*********************************************code stuck here
	//add time stamp
	addTimeStamp(buff);
	creatSMS(buff,1,0);
}

void creat_power_sms(bool power_state){
	char buffer[100];
	memset(buffer,'\0',100);
	if (power_state)
	{
		strcpy_P(buffer,PSTR("AC POWER OK"));
	}
	else
	{
		strcpy_P(buffer,PSTR("NO AC POWER!!!"));
	}
	//add time stamp
	addTimeStamp(buffer);
	creatSMS(buffer,1,"0");
	
}

//retuns
//0 - succesfully done
//1 - somethong is wrong
//cases
//0.check network
//1. init call
//2. check call success if not retry case
///3. check answer or busy
//4.play musics
//5.check hang up
//6.move to next number
uint8_t ultimate_call_hadlr(){
	static int  Prev_caller_state = -1;
	static int call_try_times=systemConfig.call_attempts;
	uint8_t ret_val=3;
	static char current_phone_number[15];
	
	const TickType_t xTicksToWait = 100 / portTICK_PERIOD_MS;
	EventBits_t uxBits;
	uxBits = xEventGroupWaitBits(
	EventRTOS_gsm,   /* The event group being tested. */
	TASK_7_BIT, /* The bits within the event group to wait for. */
	pdTRUE,        /* BITS should be cleared before returning. */
	pdFALSE,       /* Don't wait for both bits, either bit will do. */
	xTicksToWait );/* Wait a maximum of 100ms for either bit to be set. */
	if(  ( uxBits & TASK_7_BIT ) != 0  )// CALL TASK DELETE request received.
	{
		Current_caller_state=0;
		alarm_calling_index=1;
		Serial.println(F("Abort CALL TASK AS REQUESTED - HANG UP ALL CALLS"));		
			   waitForMutex_GSM();
			  if(fona.getCallStatus()==FONA_CALL_INPROGRESS)
			  {
				  fona.hangUp();
				  delay(1500);
	
			  }
			   releaseMutex_GSM();
		 xEventGroupClearBits(
		 EventRTOS_gsm,  /* The event group being updated. */
		 TASK_6_BIT);/* The bits being cleared. */
		 //resume sms task
		 uxBits = xEventGroupSetBits(EventRTOS_gsm,    TASK_5_BIT );
		
		 vTaskDelete(NULL);
	}	
	switch (Current_caller_state)
	{
	case 0:{
			if (Prev_caller_state!=Current_caller_state)
			{
				Prev_caller_state=Current_caller_state;
				Timer_alarm_calling_time_out.previousMillis=millis();
				Timer_alarm_calling_time_out.interval=180000;
				
			}
			ultimate_gsm_listiner();
			waitForMutex_GSM();
			Serial.print(F("CHECK_REGISTATION BEFOR CALL"));
			if (fona.getNetworkStatus()==1)
			{
				Current_caller_state=1;
				ret_val = 1;
				Serial.println(F("NETWORK OK"));
			}
			else{
				Serial.println(F("NETWORK BUSY!"));
				
				
			}
			
			if (Timer_alarm_calling_time_out.Timer_run())
			{
				Serial.println(F("Network not ok for calling restart GSM"));
				fona.begin(*fonaSerial);
				releaseMutex_GSM();
			}else{
				releaseMutex_GSM();
			}
			//releaseMutex_GSM();
			//
			
	}
		break;
	case 1:{
		ret_val = 1;
		if (Prev_caller_state!=Current_caller_state)
		{
			Prev_caller_state=Current_caller_state;
			Serial.println(F("DIALING>>>"));
			//call.SetDTMF(true);			
		}		
		//waitForMutex_GSM();		
		if(FONA_CALL_READY==fona.getCallStatus()){
			// next number
			memset(current_phone_number,'\0',15);
			strcpy(current_phone_number,get_GSM_number(alarm_calling_index));
				
			if ((strcmp_P(current_phone_number,PSTR("N"))==0)||(strcmp_P(current_phone_number,PSTR("+0000000000"))==0))
			{
				Current_caller_state=0;
				alarm_calling_index=1;
				Serial.println(F("No number to call"));
				Serial.println(F("Abort CALL TASK AS NO NUMBER"));
				ret_val = 0;
				releaseMutex_GSM();
				//myAlarm_pannel.set_system_state(SYS1_IDEAL,SYSTEM_ITSELF,0);
				 /* Clear call in progress bit in GSMEventGroup. */
				 xEventGroupClearBits(
				 EventRTOS_gsm,  /* The event group being updated. */
				 TASK_6_BIT);/* The bits being cleared. */
				 //resume sms task
				 uxBits = xEventGroupSetBits(EventRTOS_gsm,    TASK_5_BIT );
				 if(ALARM_CALLING == myAlarm_pannel.get_system_state()){
					myAlarm_pannel.set_system_state(SYS1_IDEAL,SYSTEM_ITSELF,0);		
				 }
				 		  
				 vTaskDelete(NULL); 			
			}
			if (!get_is_GSM_number_call(alarm_calling_index))
			{
				Serial.println(F("not a calling number"));
				alarm_calling_index++;
				Current_caller_state=1;
				
			}
			else{
				Serial.print(F("Call initiating...TP index:"));
				Serial.println(alarm_calling_index);
				fona.callPhone(current_phone_number);
				tell_lcd(current_phone_number);
				//delay(500);
				Current_caller_state=2;
			}
			
			
		}
		else if(fona.getCallStatus()==FONA_CALL_INPROGRESS)
		{
			fona.hangUp();
			Current_caller_state=0;
			releaseMutex_GSM();
		}
	}
	break;
	case 2:{
		ret_val = 1;
		if (Prev_caller_state!=Current_caller_state)
		{
			Prev_caller_state = Current_caller_state;
			Serial.println(F("checking call progress"));
			Timer_call_init_delay.previousMillis = millis();
			Timer_call_init_delay.interval=10000;
		}
		if(FONA_CALL_INPROGRESS==fona.getCallStatus()){
			Serial.print(F("Dialing Succeed"));
			Current_caller_state=3;
			//alarm_calling_index = alarm_calling_index + 1;
		}
		if (Timer_call_init_delay.Timer_run())
		{
			Serial.println(F("call init time out"));
			
			releaseMutex_GSM();
			Current_caller_state=0;
		}
	}
	break;
	
	case 3:{
		ret_val = 1;
			if (Prev_caller_state!=Current_caller_state)
			{
				Prev_caller_state=Current_caller_state;
				Serial.println(F("waiting for fedback"));	
				Timer_call_answer_delay.interval=40000;
				Timer_call_answer_delay.previousMillis=millis();							
			}
			/*const char msg_answer[15]="MO CONNECTED";
			const char msg_busy[10]="BUSY";
			const char msg_ring[10]="MO RING";
			const char msg_no_carry[12]="NO CARRIER";
			const char msg_no_dial_tone[12]="NO DIALTONE";*/		
				 uint8_t call_res = fona.waitCallResp(100);// it was 10000
					
					if (call_res ==2)
					{
						Serial.println(F("call answered"));
						Current_caller_state=4;
						 alarm_calling_index++;
						 releaseMutex_GSM();
					}
					else if (call_res ==3)
					{
						Serial.println(F("call busy"));
						Current_caller_state=0;
						alarm_calling_index++;
						releaseMutex_GSM();
					}
					else if (call_res ==1)
					{
						
						Serial.println(F("call ringing"));
						releaseMutex_GSM();
						Timer_call_answer_delay.interval=60000;
						Timer_call_answer_delay.previousMillis=millis();
						//Current_caller_state=0;
					}
					else if (call_res ==5)
					 {
						Serial.println(F("no dial tone trying again"));
						releaseMutex_GSM();
						Current_caller_state=0;
					}
					else if (call_res ==4)
					{
						releaseMutex_GSM();
						Serial.println(F("call did not answer"));
						call_try_times--;
						if (!call_try_times)
						{
							Serial.println(F("try times over moving to next number"));
							call_try_times=systemConfig.call_attempts;
							alarm_calling_index++;
						}
						Current_caller_state=0;
						/*return ret_val;*/
					}
					
					//alarm_calling_index = alarm_calling_index + 1;				
				else if (Timer_call_answer_delay.Timer_run())
				{
					Serial.println(F("no feedback from gsm, time out"));
					Current_caller_state=0;
					//alarm_calling_index=1;
					releaseMutex_GSM();
					/*return ret_val;*/
				}
		}break;
		
		case 4:{
				ret_val = 1;
				if (Prev_caller_state!=Current_caller_state)
				{
					Prev_caller_state=Current_caller_state;
					Serial.println(F("Hang up call"));
					fona.hangUp();
					Current_caller_state=0;
				}
					
			}break;			
	}	
	return ret_val;
}



bool waitForMutex_GSM(){
	//Serial.println(F("Taking GSM Mutex"));
	while (1)
	{
		if (xSemaphoreTake( xMutex_GSM, portMAX_DELAY ))
		{
			break;
		}
	}
	return true;
}

void releaseMutex_GSM(){
	//Serial.println(F("Releasing GSM Mutex"));
	xSemaphoreGive(xMutex_GSM);
}


void tell_lcd(char *number){
	const TickType_t x100ms = pdMS_TO_TICKS( 10 );
				/* Send the string to the message buffer.  Return immediately if there is
				not enough space in the buffer. */
				size_t xBytesSent;
				xBytesSent = xMessageBufferSend( xMessageBuffer_number,
											( void * ) number,
											strlen( number ), 0 );

				if( xBytesSent != strlen( number ) )
				{
					Serial.println(F("The string could not be added to the message buffer because there was not enough free space in the buffer"));
					/* . */
				}
}


void addTimeStamp(char* sms_buffer){
	strcat(sms_buffer,"\n");
	String timeString = rtc.getTime("%d(%a)-%b-%y %H:%M:%S");
	//Serial.println(timeString);
	strcat(sms_buffer,timeString.c_str());
}


void convertTime(char* input_string, char* output_string) {
	 // Copy the input string to the output string
	 strcpy(output_string, input_string+1);
	 
	 // Replace the comma with a space
	 output_string[8] = ' ';
	 
	 // Remove the timezone offset
	 output_string[17] = '\0';
 
}

void setESP32_rtc(char *timeChars){
	// convert the time string to a C string
	//char timeChars[timeString.length() + 1];
	//strcpy(timeChars, timeString.c_str());
	//<--- +CCLK: "23/04/22,12:29:49+22"
	// split the time string into its individual components
	
	 int len = strlen(timeChars);
	 char new_s[len-1];
	 strncpy(new_s, timeChars+1, len-2);
	 new_s[len-2] = '\0';
	 
	char *yearStr = strtok(new_s, "/");
	char *monthStr = strtok(NULL, "/");
	char *dayStr = strtok(NULL, ",");
	char *hourStr = strtok(NULL, ":");
	char *minuteStr = strtok(NULL, ":");
	char *secondStr = strtok(NULL, "+");
	char *tzOffsetStr = strtok(NULL, "");

	// convert the individual components to integers
	int day = atoi(dayStr);
	int month = atoi(monthStr);
	int year = atoi(yearStr) + 2000;
	int hour = atoi(hourStr);
	int minute = atoi(minuteStr);
	int second = atoi(secondStr);
	int tzOffset = atoi(tzOffsetStr);
#ifdef _DEBUG
	Serial.print("Day: ");
	Serial.println(day);

	Serial.print("Month: ");
	Serial.println(month);

	Serial.print("Year: ");
	Serial.println(year);

	Serial.print("Hour: ");
	Serial.println(hour);

	Serial.print("Minute: ");
	Serial.println(minute);

	Serial.print("Second: ");
	Serial.println(second);

	Serial.print("Time Zone Offset: ");
	Serial.println(tzOffset);
#endif

	// adjust the time components for the time zone offset
	/*hour += tzOffset;
	if (hour < 0) {
		hour += 24;
		day -= 1;
		} else if (hour >= 24) {
		hour -= 24;
		day += 1;
	}
*/

	// set the RTC using the adjusted time components
	 rtc.setTime(second, minute, hour, day, month, year);
}

