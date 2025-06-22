
#include "modbusx.h"

bool mstate = true;
ModbusMaster node;
// Pin 4 made high for Modbus transmision mode
void modbusPreTransmission()
{
  delay(5);
  //digitalWrite(MODBUS_DIR_PIN, HIGH);
}

// Pin 4 made low for Modbus receive mode
void modbusPostTransmission()
{
  //digitalWrite(MODBUS_DIR_PIN, LOW);
  delay(5);
}

void setup_modbus()
{

  // Initialize Modbus communication pins
  //pinMode(MODBUS_DIR_PIN, OUTPUT);
  //digitalWrite(MODBUS_DIR_PIN, LOW);
  Serial1.begin(19200, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);
  Serial1.setTimeout(1000);
  //modbus device slave ID 14
  node.begin(1, Serial1);
  // Callbacks allow us to configure the RS485 transceiver correctly
  node.preTransmission(modbusPreTransmission);
  node.postTransmission(modbusPostTransmission);
  Serial.println("Modbus Master - RS485 Half Duplex Example");
}


void loop_modbus()
{
  static unsigned long lastUpdate = 0;
  const unsigned long UPDATE_INTERVAL = 500; // milliseconds
  uint8_t result;

  if (millis() - lastUpdate < UPDATE_INTERVAL) return;
  lastUpdate = millis();

 // Step 1: Prepare transmit buffer
for (uint8_t i = 0; i < 8; i++) {
  node.setTransmitBuffer(i, any_sensor_array[i].device_state);
}

// Step 2: Write all 8 registers in one Modbus command
// Arguments: starting register address, number of registers
result = node.writeMultipleRegisters(0, 9);

// Step 3: Check result
if (result != node.ku8MBSuccess) {
  Serial.print("Failed to write multiple registers: ");
  Serial.println(result, HEX);
}


  // Write 8 door sensor states to slave coils 0-7
  // for (uint8_t i = 0; i < 8; i++) {
  //   //bool state = (any_sensor_array[i].device_state >> BIT_MASK_LAST_STATE) & 0x01;
  //   result = node.writeSingleRegister(i, any_sensor_array[i].device_state);
  //   // Check if the write was successful
  //   if (result != node.ku8MBSuccess) {
  //     Serial.print("Failed to write coil ");
  //     Serial.print(i);
  //     Serial.print(" : ");
  //     Serial.println(result, HEX);
  //   }
  // }

  // Write system armed/disarmed state to coil 8
  eMain_state systemState = myAlarm_pannel.get_system_state();
  result = node.writeSingleRegister(9, systemState == DEACTIVE ? 0 : 1);

  node.readHoldingRegisters(8, 1); // Read back the first 8 registers to check the state
  if (result == node.ku8MBSuccess) {
      uint8_t sysState = node.getResponseBuffer(0); // Read the first coil
      if (sysState==0x00) {
        Serial.println("System is DEACTIVE.");
        myAlarm_pannel.set_system_state(DEACTIVE, LCD, 0); // Set system state to armed
      } else if(sysState==0x01) {
        Serial.println("System is ARMED.");
        myAlarm_pannel.set_arm_mode(AS_ITIS_NO_BYPASS);
        myAlarm_pannel.set_system_state(SYS1_IDEAL, LCD, 0); // Set system state to disarmed
      }
      //node.writeSingleRegister(8, 0xff);
      node.setTransmitBuffer(8, 0xff);
    } 
  else {
    Serial.print("Error reading coils: ");
    Serial.println(result, HEX);
  }

}