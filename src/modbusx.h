#ifndef _MODBUSX_H
#define _MODBUSX_H

#include "Arduino.h"
#include "statments.h"
#include "typex.h"
#include "ModbusMaster.h"
#include "alarm.h"

/*!
  We're using a MAX485-compatible RS485 Transceiver.
  Rx/Tx is hooked up to the hardware serial port at 'Serial'.
  The Data Enable and Receiver Enable pins are hooked up as follows:
*/

/* Modbus Configuration */
#define MODBUS_DIR_PIN  5    // Pin for MAX485 DR, RE
#define MODBUS_RX_PIN 26     // Rx pin
#define MODBUS_TX_PIN 25     // Tx pin
#define MODBUS_SERIAL_BAUD 19200 // Baud rate for Modbus RTU communication

extern bool mstate;
// instantiate ModbusMaster object
extern ModbusMaster node;

// Pin 4 made high for Modbus transmision mode
void modbusPreTransmission();

// Pin 4 made low for Modbus receive mode
void modbusPostTransmission();
void setup_modbus();



void loop_modbus();

#endif // _MODBUSX_H