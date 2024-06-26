#ifndef _PINSX_H
#define _PINSX_H


//#define GSM_MINI_BOARD
#define GSM_MINI_BOARD_V2

#ifdef GSM_MINI_BOARD_V2
#define PIN_BATTERY 18
#define ANALOG_BAT A8
#define ANALOG_PWR A7
#define RELAY_ALARM 19
#define PROGRAM_PIN 25
#define PIN_RF_LED 27
#define PIN_GSM_BUSY_LED 27
#define PIN_ARM 12
#define PIN_DISARM 13
#define SENS_1 35
#define SENS_2 34
#define SENS_3 32
#define SENS_4 33
#define PIN_AC_DETECT 4
#define  BuzzerPin 2
#define GSM_LED 27
#define PIN_VOICE_EN 21
#endif


#ifdef GSM_MINI_BOARD
#define VOICE_EN 10
#define PIN_BATTERY 19
#define ANALOG_BAT A8
#define ANALOG_PWR A7
#define RELAY_ALARM 13
#define RELAY_OUT_A 16
#define REALY_OUT_B 17
#define PROGRAM_PIN 4
#define PIN_RF_LED 26
#define PIN_GSM_BUSY_LED 25
#define PIN_ARM 18
#define PIN_DISARM 23
#define SENS_1 35
#define SENS_2 34
#define SENS_3 32
#define SENS_4 33
#define CARD_A_ADDR 20
#define CARD_B_ADDR 20
#define CARD_C_ADDR 30
#define  BuzzerPin 12
#define GSM_LED 21

#endif

#ifdef GSM_FULL_BOARD
#define VOICE_EN 10
#define BATTERY_RELAY 14
#define ANALOG_BAT A8
#define ANALOG_PWR A7
#define RELAY_ALARM 27
#define RELAY_OUT_A 16
#define REALY_OUT_B 17
#define PROGRAM_PIN 4
#define PIN_RF_LED 26
#define PIN_GSM_BUSY_LED 25
#define SENS_1 14
#define SENS_2 27
#define CARD_A_ADDR 20
#define CARD_B_ADDR 20
#define CARD_C_ADDR 30
#define  BuzzerPin 12
#define GSM_LED 21
#endif

#endif