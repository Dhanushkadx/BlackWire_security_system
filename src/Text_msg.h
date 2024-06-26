/*
 * Text_msg.h
 *
 * Created: 7/7/2021 8:07:56 AM
 *  Author: dhanu
 */ 


#ifndef TEXT_MSG_H_
#define TEXT_MSG_H_

//sms sending
const char txt0[] PROGMEM ="Disarmed by ";
const char txt1[] PROGMEM ="Home armed by ";
const char txt2[] PROGMEM ="Open zones detected!";
const char txt3[] PROGMEM ="BAT V+";
const char txt4[] PROGMEM ="AC_POWER OK";
const char txt5[] PROGMEM ="CHARGING";
const char txt6[] PROGMEM ="NOT CHARGING";
const char txt7[] PROGMEM ="NO AC!";
const char txt8[] PROGMEM ="Unauthorized action";

const char txt9[] PROGMEM ="UART";
const char txt10[] PROGMEM ="LCD";
const char txt11[] PROGMEM ="CARD A";
const char txt12[] PROGMEM ="CARD B";
const char txt13[] PROGMEM ="CARD C";
const char txt14[] PROGMEM ="GSM";
const char txt15[] PROGMEM ="REMOTE";
const char txt16[] PROGMEM ="SYSTEM";
const char txt17[] PROGMEM = "UNK";
const char txt18[] PROGMEM = "Command can't be accepted.Unauthorized number";
const char txt19[] PROGMEM = "Successful";
const char txt20[] PROGMEM = "KEY PAD";





PGM_P const TXT_table_sendingSMS[] PROGMEM =
{
	txt0,
	txt1,
	txt2,
	txt3,
	txt4,
	txt5,
	txt6,
	txt7,
	txt8,
	txt9,
	txt10,
	txt11,
	txt12,
	txt13,
	txt14,
	txt15,
	txt16,
	txt17,
	txt18,
	txt19,
	txt20,
};


#define MSG1_DISARMED_BY (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[0]))
#define MSG2_HOME_ARMED_BY (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[1]))
#define MSG3_OPEN_ZONE_DETECTE (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[2]))
#define MSG4_V_BAT_plus (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[3]))
#define MSG5_AC_POWER_OK (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[4]))
#define MSG6_CHARGING (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[5]))
#define MSG7_NOT_CHARGING (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[6]))
#define MSG8_NO_AC (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[7]))
#define MSG9_UNAUTHORIZED_ACTION (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[8]))


#define MSG10_UART (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[9]))
#define MSG11_LCD (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[10]))
#define MSG12_CARD_A (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[11]))
#define MSG13_CARD_B (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[12]))
#define MSG14_CARD_C (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[13]))
#define MSG15_GSM (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[14]))
#define MSG16_REMOTE (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[15]))
#define MSG17_SYSTEM (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[16]))
#define MSG18_UNK (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[17]))
#define MSG19_CAN_NOT_ACCEPT (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[18]))
#define MSG20_SUCCESSFUL (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[19]))
#define MSG21_KEY_PAD (PGM_P)pgm_read_word(&(TXT_table_sendingSMS[20]))



//receiving sms
const char txt_com0[] PROGMEM ="Disarm";
const char txt_com1[] PROGMEM ="Home arm";
const char txt_com2[] PROGMEM ="Power";
const char txt_com3[] PROGMEM ="Number=";
const char txt_com4[] PROGMEM ="Bypass zone=";
const char txt_com5[] PROGMEM ="Entry zone=";
const char txt_com6[] PROGMEM ="Exit zone=";
const char txt_com7[] PROGMEM ="Entry delay=";
const char txt_com8[] PROGMEM ="Exit delay=";
const char txt_com9[] PROGMEM ="Zone name=";
const char txt_com10[] PROGMEM ="Sms number=";
const char txt_com11[] PROGMEM ="Call number=";
const char txt_com12[] PROGMEM ="24h zone=";

PGM_P const TXT_table_Command[] PROGMEM =
{
	txt_com0,
	txt_com1,
	txt_com2,
	txt_com3,
	txt_com4,
	txt_com5,
	txt_com6,
	txt_com7,
	txt_com8,
	txt_com9,
	txt_com10,
	txt_com11,
	txt_com12,
};



#define MSG1_DISARM (PGM_P)pgm_read_word(&(TXT_table_Command[0]))
#define MSG2_HOME_ARM (PGM_P)pgm_read_word(&(TXT_table_Command[1]))
#define MSG3_POWER (PGM_P)pgm_read_word(&(TXT_table_Command[2]))
#define MSG4_NUMBER (PGM_P)pgm_read_word(&(TXT_table_Command[3]))
#define MSG5_BYPASS_ZONE (PGM_P)pgm_read_word(&(TXT_table_Command[4]))
#define MSG6_ENTRY_ZONE (PGM_P)pgm_read_word(&(TXT_table_Command[5]))
#define MSG7_EXIT_ZONE (PGM_P)pgm_read_word(&(TXT_table_Command[6]))
#define MSG8_ENTRY_DELAY (PGM_P)pgm_read_word(&(TXT_table_Command[7]))
#define MSG9_EXIT_DELAY (PGM_P)pgm_read_word(&(TXT_table_Command[8]))
#define MSG10_ZONE_NAME (PGM_P)pgm_read_word(&(TXT_table_Command[9]))
#define MSG11_SMS_NUMBER (PGM_P)pgm_read_word(&(TXT_table_Command[10]))
#define MSG12_CALL_NUMBER (PGM_P)pgm_read_word(&(TXT_table_Command[11]))
#define MSG13_24H_ZONE (PGM_P)pgm_read_word(&(TXT_table_Command[12]))


//MCU2 I@C commands
const char I2C_txt0[] PROGMEM ="disarm";
const char I2C_txt1[] PROGMEM ="homearm";
const char I2C_txt2[] PROGMEM ="awayarm";
const char I2C_txt3[] PROGMEM ="rfs";// rf scan
const char I2C_txt4[] PROGMEM ="rfz";// rf zone
const char I2C_txt5[] PROGMEM ="rfr";// rf remote
const char I2C_txt6[] PROGMEM ="rfb";// rf death
const char I2C_txt7[] PROGMEM ="panic";// rf death
const char I2C_txt8[] PROGMEM = "rf";
const char I2C_txt9[] PROGMEM = "gsm";
const char I2C_txt10[] PROGMEM = "rfd";
const char I2C_txt11[] PROGMEM = "eerst";;
const char I2C_txt12[] PROGMEM = "xten";
const char I2C_txt13[] PROGMEM = "eten";
const char I2C_txt14[] PROGMEM = "es";
const char I2C_txt15[] PROGMEM = "alarm";
const char I2C_txt16[] PROGMEM = "zupdate";
const char I2C_txt17[] PROGMEM = "albuzz";
const char I2C_txt18[] PROGMEM = "ofbuzz";
const char I2C_txt19[] PROGMEM = "armbuzz";
const char I2C_txt20[] PROGMEM = "disbuzz";
const char I2C_txt21[] PROGMEM = "v_bat";

PGM_P const I2C_TXT_table_Command[] PROGMEM =
{
	I2C_txt0,
	I2C_txt1,
	I2C_txt2,
	I2C_txt3,
	I2C_txt4,
	I2C_txt5,
	I2C_txt6,
	I2C_txt7,
	I2C_txt8,
	I2C_txt9,
	I2C_txt10,
	I2C_txt11,
	I2C_txt12,
	I2C_txt13,
	I2C_txt14,
	I2C_txt15,
	I2C_txt16,
	I2C_txt17,
	I2C_txt18,
	I2C_txt19,
};


#define MSG1_I2C_DISARM (PGM_P)pgm_read_word(&(I2C_TXT_table_Command[0]))
#define MSG2_I2C_HOME_ARM (PGM_P)pgm_read_word(&(I2C_TXT_table_Command[1]))
#define MSG3_I2C_AWAY_ARM (PGM_P)pgm_read_word(&(I2C_TXT_table_Command[2]))

#define MSG4_I2C_ETEN (PGM_P)pgm_read_word(&(I2C_TXT_table_Command[13]))
#define MSG5_I2C_OFF_BUZ (PGM_P)pgm_read_word(&(I2C_TXT_table_Command[18]))
#define MSG6_I2C_DISBUZ (PGM_P)pgm_read_word(&(I2C_TXT_table_Command[20]))
#define MSG7_I2C_ALBUZ (PGM_P)pgm_read_word(&(I2C_TXT_table_Command[17]))
#define MSG8_I2C_ZUPDATE (PGM_P)pgm_read_word(&(I2C_TXT_table_Command[16]))
#define MSG9_I2C_VBAT (PGM_P)pgm_read_word(&(I2C_TXT_table_Command[21]))

#endif /* TEXT_MSG_H_ */