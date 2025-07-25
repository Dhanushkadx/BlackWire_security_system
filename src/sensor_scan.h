

#ifndef _SENSOR_H
#define _SENSOR_H

#include "Arduino.h"
#include "statments.h"
#include "pinsx.h"
#include "typex.h"
#include "TimerSW.h"
#include "Wire.h"
extern xQueueHandle sensorEventQueue;
extern bool on_boot_A;
extern bool on_boot_B;
extern bool on_boot_C;

extern SemaphoreHandle_t sensorMutex;

extern const uint8_t sensor_pin_count;

extern Struct_GPIO_INFO GPIO_array[];

extern Struct_SENS_INFO SENS_array[];

extern TimerSW sensroTimerArray[];

extern systemConfigTypedef_struct systemConfig;

void setup_sensor_settings();

void transfer_sensor_scan_data(const char* sensor_data);
void pooling_i2c_a();
void pooling_i2c_b();
void pooling_i2c_c();
void sensor_update_LL(uint8_t scan_index , bool state, bool force_update);
void GPIO_sens_scan();

bool getSensor(uint8_t index);

#endif
 