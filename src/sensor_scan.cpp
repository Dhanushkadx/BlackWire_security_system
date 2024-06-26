
#include "sensor_scan.h"

const uint8_t sensor_pin_count = 4;
const uint8_t sensor_count = 48;

bool on_boot_A = true;
bool on_boot_B = true;
bool on_boot_C = true;

#define CARD_A_START_INDEX 8
#define CARD_A_END_INDEX 24
#define CARD_A_CHANNELS 16

#define CARD_B_START_INDEX 24
#define CARD_B_END_INDEX 40
#define CARD_B_CHANNELS 16

#define CARD_C_START_INDEX 40
#define CARD_C_END_INDEX 28
#define CARD_C_CHANNELS 16

const int SLAVE_ADDRESS_A = 0x12; // Address of the I2C slave device
const int SLAVE_ADDRESS_B = 0x13; // Address of the I2C slave device
const int SLAVE_ADDRESS_C = 0x14; // Address of the I2C slave device

int16_t previousData_a = 0; // Variable to store previous 16-bit number
int16_t previousData_b = 0; // Variable to store previous 16-bit number
int16_t previousData_c = 0; // Variable to store previous 16-bit number

Struct_GPIO_INFO GPIO_array[sensor_pin_count];

TimerSW sensroTimerArray[sensor_count];

Struct_SENS_INFO SENS_array[sensor_count];


void setup_sensor_settings(){

	// defaultly all sensors are disable
	for(uint8_t index=0; index<48; index++){
		any_sensor_array[index].device_state &= ~(1 << BIT_MASK_AVAILABLE);
		sensroTimerArray[index].interval = systemConfig.sensor_debounce_time;
		sensroTimerArray[index].previousMillis = millis();
	}

	// setup onboard gpio
	GPIO_array[0].GPIOpin=SENS_1;
	GPIO_array[1].GPIOpin=SENS_2;
	GPIO_array[2].GPIOpin=SENS_3;
	GPIO_array[3].GPIOpin=SENS_4;

	for(uint8_t index=0; index<sensor_pin_count; index++){
		pinMode(GPIO_array[index].GPIOpin,INPUT_PULLUP);
		sensroTimerArray[index].interval = systemConfig.sensor_debounce_time;
		sensroTimerArray[index].previousMillis = millis();
		any_sensor_array[index].device_state |= (1 << BIT_MASK_AVAILABLE);
	}
	
	// defaultly all cards are disable
	systemConfig.card_A_en = false;
	systemConfig.card_B_en = false;
	systemConfig.card_C_en = false;
	systemConfig.card_D_en = false;
	systemConfig.card_E_en = false;
	systemConfig.card_F_en = false;


	// detect card A
	Wire.requestFrom(SLAVE_ADDRESS_A, 2);

	if (Wire.available() >= 2) {
		// activate sensors
	Serial.println(F("CARD A Detected"));
	systemConfig.card_A_en = true;
	for(uint8_t index=CARD_A_START_INDEX; index<CARD_A_END_INDEX; index++){
		any_sensor_array[index].device_state |= (1 << BIT_MASK_AVAILABLE);
	}
	}

	// detect card B
	Wire.requestFrom(SLAVE_ADDRESS_B, 2);

	if (Wire.available() >= 2) {
		// activate sensors
	Serial.println(F("CARD A Detected"));
	systemConfig.card_B_en = true;
	for(uint8_t index=CARD_B_START_INDEX; index<CARD_B_END_INDEX; index++){
		any_sensor_array[index].device_state |= (1 << BIT_MASK_AVAILABLE);
		Serial.printf("activating sensor:%d",index);
	}
	}

	// detect card C
	Wire.requestFrom(SLAVE_ADDRESS_C, 2);

	if (Wire.available() >= 2) {
		// activate sensors
	Serial.println(F("CARD A Detected"));
	systemConfig.card_C_en = true;
	for(uint8_t index=CARD_C_START_INDEX; index<CARD_C_END_INDEX; index++){
		any_sensor_array[index].device_state |= (1 << BIT_MASK_AVAILABLE);
	}
	}
}


// Sacan GPIO connected sensors only
void GPIO_sens_scan(){
	//Serial.println("pin change");
	for(uint8_t scan_index=0; scan_index<sensor_pin_count; scan_index++){
		bool state = digitalRead(GPIO_array[scan_index].GPIOpin);
		sensor_update_LL(scan_index, state);
	}
}


void pooling_i2c_a(){

if(systemConfig.card_A_en==false){return;}// if not enabled dont scan i2c sensors

  Wire.requestFrom(SLAVE_ADDRESS_A, 2);

  if (Wire.available() >= 2) {
    byte highByte = Wire.read();
    byte lowByte = Wire.read();
    int16_t receivedData = (highByte << 8) | lowByte;

      for (int i = 0; i < CARD_A_CHANNELS; i++) {
       bool state = (receivedData >> i) & 0x01 ? true : false;
	   sensor_update_LL(i+CARD_A_START_INDEX , state);
      }        
    
  }else{
	//publish_system_state("i2c_err","info/IOcard_A",true);
	
	}
}


void pooling_i2c_b(){

if(systemConfig.card_B_en==false){return;}// if not enabled dont scan i2c sensors

  Wire.requestFrom(SLAVE_ADDRESS_B, 2);

  if (Wire.available() >= 2) {
    byte highByte = Wire.read();
    byte lowByte = Wire.read();
    int16_t receivedData = (highByte << 8) | lowByte;  
      for (int i = 0; i < CARD_B_CHANNELS; i++) {
       bool state = (receivedData >> i) & 0x01 ? true : false;
	   sensor_update_LL(i+CARD_B_START_INDEX , state);
      }        
    
  }else{
	//publish_system_state("i2c_err","info/IOcard_B",true);	
	}
}

void pooling_i2c_c(){

if(systemConfig.card_C_en==false){return;}// if not enabled dont scan i2c sensors

  Wire.requestFrom(SLAVE_ADDRESS_C, 2);

  if (Wire.available() >= 2) {
    byte highByte = Wire.read();
    byte lowByte = Wire.read();
    int16_t receivedData = (highByte << 8) | lowByte;    
      for (int i = 0; i < CARD_C_CHANNELS; i++) {
       bool state = (receivedData >> i) & 0x01 ? true : false;
	   sensor_update_LL(i+CARD_C_START_INDEX , state);
      }        
    
  }else{
	//publish_system_state("i2c_err","info/IOcard_C",true);
	
	}
}

void transfer_sensor_scan_data(const char* sensor_data){
	/* keep the status of sending data */
	BaseType_t xStatus;
	/* time to block the task until the queue has free space */
	const TickType_t xTicksToWait = pdMS_TO_TICKS(30);
	/* create data to send */
	DataBuffer data;
	/* sender 1 has id is 1 */
	memset(data.char_buffer_rx,'\0',15);
	strcpy(data.char_buffer_rx,sensor_data);
	data.counter = 1;
	
	Serial.println(F("sensor data Q is sending data"));
	/* send data to front of the queue */
	xStatus = xQueueSendToFront( xQueue_sensor_state, &data, xTicksToWait );
	/* check whether sending is ok or not */
	if( xStatus == pdPASS ) {
		/* increase counter of sender 1 */
		data.counter = data.counter + 1;
		Serial.println(F("send OK"));
	}
	/* we delay here so that receiveTask has chance to receive data */
	delay(100);
}


void sensor_update_LL(uint8_t scan_index , bool state){		
	bool send_data_ready = false;
	char buffer[20] = {0};
	if (xSemaphoreTake(sensorMutex, portMAX_DELAY) == pdTRUE) {	 
		SENS_array[scan_index].nowState = state;
		if(SENS_array[scan_index].prevState!=SENS_array[scan_index].nowState){// there is a change
			long now = millis(); // get time 
			int duration = now-SENS_array[scan_index].last_changed_time;// diffrence of time with last change
			SENS_array[scan_index].last_changed_time = now;
			SENS_array[scan_index].prevState = SENS_array[scan_index].nowState;
			if(!SENS_array[scan_index].nowState){
				SENS_array[scan_index].high_satate_triggerd = false;
				//sensor is low
				//Serial.printf("sensor low:%d",scan_index);
				if(scan_index<10){sprintf_P(buffer,PSTR("sc=0%d,0"),scan_index);}
				else{sprintf_P(buffer,PSTR("sc=%d,0"),scan_index);}
				send_data_ready = true;
				
				
			}
			else{
				sensroTimerArray[scan_index].previousMillis = millis();
				SENS_array[scan_index].high_satate_triggerd = true;

			}
		}
		else{
			if(!SENS_array[scan_index].nowState){
			
			}
			else {
				if(SENS_array[scan_index].high_satate_triggerd == true){
					if(sensroTimerArray[scan_index].Timer_run()){
					SENS_array[scan_index].high_satate_triggerd = false;
					Serial.printf("sensor high:%d",scan_index);
					
					if(scan_index<10){sprintf_P(buffer,PSTR("sc=0%d,1"),scan_index);}
					else{sprintf_P(buffer,PSTR("sc=%d,1"),scan_index);}
					send_data_ready = true;
					
				}
			}
			}
		}

		// Unlock the mutex after accessing the shared struct
    	xSemaphoreGive(sensorMutex);

	}	

	
	if(send_data_ready){
		send_data_ready = false;
		transfer_sensor_scan_data(buffer);
	}
	
}


bool getSensor(uint8_t index){

	 while(!(xSemaphoreTake( sensorMutex, portMAX_DELAY )));
	 bool state =  SENS_array[index].nowState;
	 // Unlock the mutex after accessing the shared struct
     xSemaphoreGive(sensorMutex);
	 return state;
}