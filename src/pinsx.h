#ifndef _PINSX_H
#define _PINSX_H


//#define GSM_MINI_BOARD
//#define GSM_PULSEX_IOT_BOARD
#define GSM_MINI_BOARD_V3

#ifdef GSM_MINI_BOARD_V2
#define PIN_BATTERY 18
#define ANALOG_BAT A8
#define ANALOG_PWR A7
#define RELAY_ALARM 19
#define PIN_RF_LED 27
#define PIN_GSM_BUSY_LED 27
#define PIN_ARM 12
#define PIN_DISARM 13
#define SENS_1 35
#define SENS_2 34
#define SENS_3 32
#define SENS_4 33
#define PIN_AC_DETECT 4
#define BuzzerPin 2
#define GSM_LED 27
#define PIN_VOICE_EN 21
#define GSM_SERIAL Serial1
#define GSM_TX 16
#define GSM_RX 17
#define FONA_RST 4
#define PIN_RF433MH 15
#endif

#ifdef GSM_MINI_BOARD_V3
#define PIN_BATTERY 18
#define ANALOG_BAT A8
#define ANALOG_PWR A7
#define RELAY_ALARM 19
#define PROGRAM_PIN 39
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
#define RELAY_OUT_A 25
#define RELAY_OUT_B 26
//#define PIN_VOICE_EN 21
#define GSM_SERIAL Serial1
#define GSM_TX 16
#define GSM_RX 17
#define FONA_RST 5
#define PIN_RF433MH 15
#endif


#ifdef GSM_MINI_BOARD
#define PIN_VOICE_EN 10
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
#define PIN_AC_DETECT 39
#define PIN_RF433MH 15
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
#define PIN_RF433MH 15
#endif

#ifdef GSM_PULSEX_IOT_BOARD
//define 4 input pins as INPUT_PIN_x
#define INPUT_PIN_1 35
#define INPUT_PIN_2 34
#define INPUT_PIN_3 36
#define INPUT_PIN_4 39

//define 6 output pins as OUTPUT_PIN_x
#define OUTPUT_PIN_1 13
#define OUTPUT_PIN_2 27
#define OUTPUT_PIN_3 2
#define OUTPUT_PIN_4 18
#define OUTPUT_PIN_5 19
//OUTPUT_PIN_6 is not used for FONA RST pin

#define RELAY_ALARM OUTPUT_PIN_1
#define PROGRAM_PIN 4
#define PIN_RF_LED OUTPUT_PIN_4
#define PIN_GSM_BUSY_LED OUTPUT_PIN_4
#define PIN_ARM OUTPUT_PIN_2
#define PIN_DISARM OUTPUT_PIN_3
#define SENS_1 INPUT_PIN_1
#define SENS_2 INPUT_PIN_2
#define SENS_3 INPUT_PIN_3
#define SENS_4 INPUT_PIN_4  
#define PIN_AC_DETECT 21
#define BuzzerPin OUTPUT_PIN_5

#define GSM_SERIAL Serial1
#define GSM_TX 32
#define GSM_RX 33
#define FONA_RST 22
#define PIN_RF433MH 4

#endif

#endif