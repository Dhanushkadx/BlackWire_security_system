#include "siren.h"


TimerSW Timer_siren;
TimerSW Timer_buzzer;
EventGroupHandle_t EventRTOS_buzzer;
EventGroupHandle_t EventRTOS_siren;

void relayTask() {

  static eSiren_state eCurrent_siren_state = SIREN_OFF, ePrev_siren_state = SIREN_OFF;
		const TickType_t xTicksToWait = pdMS_TO_TICKS(1);
		EventBits_t uxBits;
		uxBits = xEventGroupWaitBits(
		EventRTOS_siren,   /* The event group being tested. */
		ALL_SYNC_BITS, /* The bits within the event group to wait for. */
		pdTRUE,        /* BITS should be cleared before returning. */
		pdFALSE,       /* Don't wait for all bits, either bit will do. */
		xTicksToWait );/* Wait a maximum of 100ms for either bit to be set. */
		
		
		  if(  ( uxBits & TASK_1_BIT ) != 0  )// siren off
		  {			  
			  Serial.println(F("EVENT SIREN_OFF"));
			  eCurrent_siren_state = SIREN_OFF;
			  call_back_alarm_bell_time_out();
			  
		  }
		  if(  ( uxBits & TASK_2_BIT ) != 0  )// first siren
		  {
			  if(ePrev_siren_state==SIREN_OFF){
			   Serial.println(F("EVENT SIREN_MOMENT"));
			   eCurrent_siren_state = SIREN_MOMENT;
			   Timer_siren.interval = 1500;
			   Timer_siren.previousMillis = millis();
			   digitalWrite(RELAY_ALARM,HIGH);
			  }
			  else if (ePrev_siren_state==SIREN_WAIT_INTERVAL){
				eCurrent_siren_state = SIREN_CONTINUOUS;
			  }
		  }
		   if(  ( uxBits & TASK_3_BIT ) != 0  )// second siren
		   {
			   
			   Serial.println(F("EVENT SIREN_CONTINUE")); 			  
		   }		  
		  
		switch (eCurrent_siren_state)
		{
		case SIREN_OFF:{
			if (ePrev_siren_state!=eCurrent_siren_state)
			{
				ePrev_siren_state=eCurrent_siren_state;
				Serial.println(F("SIREN_OFF"));
				
			}
			digitalWrite(RELAY_ALARM,LOW);
		}
			break;
		case SIREN_MOMENT:{
			if (ePrev_siren_state!=eCurrent_siren_state)
			{
				ePrev_siren_state=eCurrent_siren_state;
				Serial.println(F("SIREN_MOMENT"));
			}
			if(Timer_siren.Timer_run()){
				 digitalWrite(RELAY_ALARM,LOW);
				 eCurrent_siren_state = SIREN_WAIT_INTERVAL;
				 Timer_siren.previousMillis = millis();
				 Timer_siren.interval = 20000;
				 Serial.println(F("FIRTS SIREN DONE!"));
			}
		}
		break;
		case SIREN_WAIT_INTERVAL:{
			if (ePrev_siren_state!=eCurrent_siren_state)
			{
				ePrev_siren_state=eCurrent_siren_state;
				Serial.println(F("SIREN_INTERVAL"));
			}
			if(Timer_siren.Timer_run()){// wait for interval before do second siren
				eCurrent_siren_state = SIREN_CONTINUOUS;
			}

		}
		break;
		case SIREN_CONTINUOUS:{
			if (ePrev_siren_state!=eCurrent_siren_state)
			{
				ePrev_siren_state=eCurrent_siren_state;
				digitalWrite(RELAY_ALARM,HIGH);
				Timer_siren.interval = systemConfig.bell_time_out*1000;
				Timer_siren.previousMillis = millis();
				Serial.println(F("SIREN_CONTINUE"));
			}
			if(Timer_siren.Timer_run()){// wait for second siren duration
				eCurrent_siren_state = SIREN_OFF;
				alarm_bell_time_out();
			}

		}
		break;
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
		  if(  ( uxBits & TASK_2_BIT ) != 0  )// Alarm triggerd
		  {
			  
			   Serial.println(F("EVENT BUZZER_ALARM"));
			   eCurrent_buzzer_state = BUZZER_ALARM;
		  }
		   if(  ( uxBits & TASK_3_BIT ) != 0  )// 
		   {
			   
			   Serial.println(F("EVENT BUZZER_RF"));
			  if(eCurrent_buzzer_state != BUZZER_ALARM){ 
					eCurrent_buzzer_state = BUZZER_RF;
					ePrev_buzzer_state = BUZZER_OFF;// bug fix for no buzzer sound when sensor trigger in disarm mode
				}			   
			  
		   }
		    if(  ( uxBits & TASK_4_BIT ) != 0  )// 
		    {			    
			    Serial.println(F("EVENT BUZZER_GSM_ERROR"));
			    eCurrent_buzzer_state = BUZZER_GSM_ERROR;
		    }
			if(  ( uxBits & TASK_5_BIT ) != 0  )// 
			{				
				Serial.println(F("EVENT BUZZER_ARM"));
				eCurrent_buzzer_state = BUZZER_ARM;
				arm_busy_buzzer();
				eCurrent_buzzer_state = BUZZER_OFF;
			}
			if(  ( uxBits & TASK_6_BIT ) != 0  )// Disarm
			{
				
				Serial.println(F("EVENT BUZZER_DISARM"));
				eCurrent_buzzer_state = BUZZER_DISARM;
				disarm_busy_buzzer();
				eCurrent_buzzer_state = BUZZER_OFF;
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
				Timer_buzzer.previousMillis = millis();
				Timer_buzzer.interval = 5000;
			}
			digitalWrite(BuzzerPin,HIGH);
			delay(180);
			digitalWrite(BuzzerPin,LOW);
			delay(180);
			if(Timer_buzzer.Timer_run()){
				eCurrent_buzzer_state = BUZZER_OFF;
			}
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


void alarm_bell_time_out(){
	Serial.println(F("Bell timeout"));
	char buffer2[50];
	sprintf(buffer2,"ALARM Muted in %d seconds!",systemConfig.bell_time_out);
	
	creatSMS(buffer2,1,0);	

}
	