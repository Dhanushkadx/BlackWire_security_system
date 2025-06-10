
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

bool gsm_init(){
	

	 Serial.println(F("FONA basic test"));
	 Serial.println(F("Initializing....(May take 3 seconds)"));

	 fonaSerial->begin(9600, SERIAL_8N1, 16, 17);
	 if (! fona.begin(*fonaSerial)) {
		 Serial.println(F("Couldn't find FONA"));
		 //while (1);
		 return false;
	 }
#ifdef _DEBUG
	 type = fona.type();
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
	 setTime_from_gsm();
	 
	 
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
			
		}		
		
	 }	 
	 thisIs_Restart = false;
#endif	 
		return true;
	 
}

uint8_t getSignal_strength(){

	return  gsmsignal_rssi;
}

void setTime_from_gsm(){
	char networkTime[100];
	// char localTime[20];
	if (fona.getTime(networkTime, 100)) {
		 setESP32_rtc(networkTime);
	 }
}

void creatSMS(const char* buffer,uint8_t type, const char* number){// creat a SMS
	int msg_length = strlen(buffer);
	char sms_159[159]="";
	if (msg_length>155)
	{
		Serial.println(F("msg is beyond the size of sms. so Split it"));
		strlcpy(sms_159,buffer,155);
		//creatSMS_LL(sms_159,type,0),number;
		creatSMS_LL(sms_159,type,number);
		creatSMS_LL(buffer+155,type,number);
		
	}
	else{
		creatSMS_LL(buffer,type, number);
	}
	DynamicJsonDocument doc(2048);
	doc["msgTyp"] = type;
  	doc["msg"] = buffer;
	saveMessageToSPIFFSV3(doc);
	
}

void creatSMS_LL(const char* buffer,uint8_t type,const char* number){
	
	if (sms_buffer_msg_count>=SMS_STRCUT_MAX_MGS)
	{
		sms_buffer_msg_count =0;
		Serial.println(F("no memory!!!!!!!!!!"));
		return;
	}
	
	SMS_to_be_sent_FIXDMEM[sms_buffer_msg_count].type = type;	
	SMS_to_be_sent_FIXDMEM[sms_buffer_msg_count].msg_content = buffer;
	SMS_to_be_sent_FIXDMEM[sms_buffer_msg_count].number = number;


	Serial.print(F("creating SMS>"));
	Serial.println(buffer);
	sms_buffer_msg_count = sms_buffer_msg_count + 1;
	delay(1);
	sms_broadcast_request = true;	
}





void ultimate_sms_hadlr(){
	
	static int Current_smsHdlr_state = 0, Prev_smsHdlr_state = -1;
	static int now_sending_index = 0 ;
	static int sms_send_try_time=0;
	static boolean invalid_phone_number=true;
	static char num[13];
	
	switch (Current_smsHdlr_state)
	{
		case 0:{
		if (Prev_smsHdlr_state!= Current_smsHdlr_state)
		{
			Prev_smsHdlr_state = Current_smsHdlr_state;
			Serial.println(F("SMS TASK 1st Stage"));
			
		}
		
		bool should_I_be_suspend = false; // NO
		bool is_suspend_request =  check_SMS_TASK_suspend_request();
		if (is_suspend_request)
		{
			 Serial.println(F("AS REQUESTED SMS TASK SUSPENDING..."));
			 should_I_be_suspend = true;
		}
		
		 if (!sms_sending_queu_complete)
		 {
				 Serial.println(F("SMS TASK SUSPENDED IT SELF AS NO NUMBER"));
				 should_I_be_suspend = true;
				 sms_sending_queu_complete = true;//**********************************
				
		  }
		  
		  if (should_I_be_suspend)
		  {
			   xEventGroupSetBits(EventRTOS_gsm,    TASK_4_BIT );// sms task suspend
			   vTaskSuspend(NULL);
			   should_I_be_suspend = false;
		  }
			
		     
			 Current_smsHdlr_state = 1;
			 
			
			
		}break;
		case 1:{
				if (Prev_smsHdlr_state!= Current_smsHdlr_state)
				{
					Prev_smsHdlr_state = Current_smsHdlr_state;
					Serial.println(F("SMS TASK 2st Stage"));
					memset(num,'\0',13);
					//send a sms to a contact that is no the contact list
					if (SMS_to_be_sent_FIXDMEM[now_sending_index].type==4)
					{
						Serial.println(F("Only send to mqtt request contact id!"));
						strcpy(num,SMS_to_be_sent_FIXDMEM[now_sending_index].number.c_str());//+94714427691
						//num[12]='\0';
						if (!phone_number_validat(num))// validating number
						{
							Serial.println(F("Only for mqtt request num but its an invalid number"));
							//Current_smsHdlr_state=0;
							//sms_broad_cast_request=false;
							Current_smsHdlr_state=2;					
						}
						else{// we can go to TX stage
							Current_smsHdlr_state = 4;
						}
					}
					// check sms type if type is 3 only send to last sender			
					else if (SMS_to_be_sent_FIXDMEM[now_sending_index].type==3)
					{
						Serial.println(F("Only send to last sender!"));
						strcpy(num,systemConfig.last_sms_sender);//+94714427691
						if (!phone_number_validat(num))// validating number
						{
							Serial.println(F("Only for last num but its a an invalid number"));
							//Current_smsHdlr_state=0;
							//sms_broad_cast_request=false;
							Current_smsHdlr_state=2;					
						}
						else{// we can go to TX stage
							Current_smsHdlr_state = 4;
						}
					}
		
					else{
				
					//check number
					strcpy(num,get_GSM_number(sms_broadcast_index));//+94714427691
					char* ch = strstr("N",num);// validating number
					if (ch!=NULL)
					{				
						Serial.println(F("all complete for the given number"));
						Current_smsHdlr_state=2;
					}							
					else if (!phone_number_validat(num))// validating number
					{
						Serial.println(F("Validating phone number Error! go to next number"));
						sms_broadcast_index++;
						Current_smsHdlr_state=0;	
					}						
					else if (!get_is_GSM_number_sms(sms_broadcast_index))
					{
						Serial.println(F("not a sms number"));
						sms_broadcast_index++;
						Current_smsHdlr_state=0;	
					}
					else{
						Current_smsHdlr_state=4;
					}
					}			
				}
					
			
		}break;
		case 2:{
				if (Prev_smsHdlr_state!= Current_smsHdlr_state)
				{
					Prev_smsHdlr_state = Current_smsHdlr_state;
					Serial.print(F("CHECK NEXT SMS INDEX: "));
					now_sending_index++;
					
					if (now_sending_index>=sms_buffer_msg_count)
					{
						Current_smsHdlr_state=0;
						now_sending_index = 0;
						sms_sending_queu_complete=false;
						sms_buffer_msg_count=0;
						sms_broadcast_index=1;
						Serial.println(F("NO SMS"));
					}
					else{
						sms_broadcast_index=1;// new msg should always be started from 1 st number
						//Current_smsHdlr_state=4;// go to tx stage
						Current_smsHdlr_state=1;// start tx from the begining og num list
						Serial.println(now_sending_index);
						
					}
				}
		}
		break;
		
		case 3:{
				if (Prev_smsHdlr_state!=Current_smsHdlr_state)
				{
					Prev_smsHdlr_state=Current_smsHdlr_state;					
				}											
						Current_smsHdlr_state=0;					
						sms_send_try_time=0;						
						if (SMS_to_be_sent_FIXDMEM[now_sending_index].type==2)// only send to programmer so take the next sms
						{
							Serial.println(F("This sms is only for programmer"));
							Current_smsHdlr_state=2;
							sms_broadcast_index = 1;						
						}
						else if (SMS_to_be_sent_FIXDMEM[now_sending_index].type==3)// only send to last sender so take the next sms
						{
							Serial.println(F("This sms is only for last sender"));
							Current_smsHdlr_state=2;
							sms_broadcast_index = 1;							
						}
						else if (SMS_to_be_sent_FIXDMEM[now_sending_index].type==4)// This sms is only for contact id
						{
							Serial.println(F("This sms is only for contact id"));
							Current_smsHdlr_state=2;
							sms_broadcast_index = 1;							
						}
						else{
							//get next number
							if (sms_broadcast_index<8)
							{
								Serial.println(F("GET NEXT NUMBER"));
								sms_broadcast_index++;
								Current_smsHdlr_state=0;
							}
							else{
								Current_smsHdlr_state=2;// go to next sms
								
							}
						}			
					
			}break;
			
			case 4:{
					if (Prev_smsHdlr_state!=Current_smsHdlr_state)
					{
						Prev_smsHdlr_state=Current_smsHdlr_state;
						Timer_sms_send_delay.previousMillis = millis();
					}
					/*char num[13]="";
					strlcpy(num,systemConfig.last_sms_sender,13);//+94714427691
					num[12]='\0';*/
					
					Serial.print(F("Transmitting SMS to>"));
					Serial.print(num);
					Serial.print(F("sms>"));
					Serial.println(SMS_to_be_sent_FIXDMEM[now_sending_index].msg_content.c_str());
					// To send an SMS, call modem.sendSMS(SMS_TARGET, smsMessage)
					String smsMessage = SMS_to_be_sent_FIXDMEM[now_sending_index].msg_content.c_str();
					// send an SMS!
					char sendto[21], message[141];
					strcpy(message,SMS_to_be_sent_FIXDMEM[now_sending_index].msg_content.c_str());		
					waitForMutex_GSM();	
					if (!fona.sendSMS(num, message)) {
							
						Serial.println(F("Failed"));
						// wait some time
						delay(5000);
						Current_smsHdlr_state=4;
					} 
					else {
						Serial.println(F("Sent!"));
						Current_smsHdlr_state=3;				 
						}
					
					if (Timer_sms_send_delay.Timer_run())
					{
						Serial.println(F("GSM re Init"));
						 if (! fona.begin(*fonaSerial)) {
							 Serial.println(F("Couldn't find FONA"));							
						 }
					}
					releaseMutex_GSM();
			}
			break;
			
			case 5:{
						if (Prev_smsHdlr_state!=Current_smsHdlr_state)
						{
							Prev_smsHdlr_state=Current_smsHdlr_state;
							Serial.println(F("SMS SEND RETRY"));
							
						}
							if(fona.getNetworkStatus()==1){
								digitalWrite(GSM_LED,HIGH);
								// Serial.println(F("Network ok"));
								Current_smsHdlr_state = 4;
							
							}
							else{
								//  Serial.println(F("Not Reg"));
								digitalWrite(GSM_LED,LOW);
								// Write time out
							}
							//check network state;
						
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
			
			/*		
			if (!phone_number_validat(num))// validating number
			{
				Serial.println(F("Validating phone number Error! go to next number"));
				sms_broadcast_index++;
				Current_smsHdlr_state=0;
				return;				
			}						
			if (!get_is_GSM_number_sms(sms_broadcast_index))
			{
				Serial.println(F("not a sms number"));
				sms_broadcast_index++;
				Current_smsHdlr_state=0;
				return;				
			}*/
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
	 if(fona.getNetworkStatus()==1){
		 digitalWrite(GSM_LED,HIGH);
		 gsmsignal_rssi = fona.getRSSI();
		 Serial.printf_P(PSTR("Network ok> RSSI: %d\n"), gsmsignal_rssi);
 
		 if(timesync_need== true){
			timesync_need=false;
			setTime_from_gsm();
		 }

	 }	 
	 else{
		  Serial.println(F("Not Reg"));
		  digitalWrite(GSM_LED,LOW);
		  releaseMutex_GSM();
		  return false;
	 }
	 /* if (fona.getCallStatus()==3)
	  {
		  Serial.println(F("there is incoming call I discarded it"));
		  fona.hangUp();
	  }	*/
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
//#ifdef _DEBUG
			  Serial.println(n);
			  Serial.printf_P(PSTR("SMS>%s"),local_smsbuffer);
//#endif	  
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


void gsm_manager(){
	
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
				digitalWrite(PIN_GSM_BUSY_LED,LOW);
			}
			if(gsm_init()){

				eCurruntGSM_state = GSM_LISTIN;
				Serial.println(F("GSM initializing OK"));
			}

		} break;
		case GSM_LISTIN:{
			if (ePrevGSM_state!=eCurruntGSM_state)
			{
				ePrevGSM_state = eCurruntGSM_state;
				Serial.println(F("GSM_LISTIN"));
				digitalWrite(PIN_GSM_BUSY_LED,LOW);
			}
			digitalWrite(PIN_GSM_BUSY_LED,HIGH);
			//Serial.println(digitalRead(PROGRAM_PIN));
			bool gsm_state =  ultimate_gsm_listiner();
			if(!gsm_state){ }// network erro count to reset gsm module
			digitalWrite(PIN_GSM_BUSY_LED,LOW);

			 if(sms_broadcast_request == true){
				Serial.println(F("sms task invoike"));
				sms_broadcast_request = false;
				sms_sending_queu_complete = true;
				EventBits_t uxBits;
				uxBits = xEventGroupSetBits(EventRTOS_gsm,    TASK_5_BIT );// SMS task invoking request
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
				digitalWrite(PIN_GSM_BUSY_LED,LOW);
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
				digitalWrite(PIN_GSM_BUSY_LED,HIGH);
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
			 if (!digitalRead(PROGRAM_PIN))// give full access if the program pin is low
			 {
				  systemConfig.cli_access_level = 3;
			 }
			 
			 
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
	creatSMS(buffer,1,0);
	
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
	Serial.println(F("Taking GSM Mutex"));
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
	Serial.println(F("Releasing GSM Mutex"));
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


/*
switch ()
{
case :
	break;
}*/