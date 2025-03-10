/*
 * config_JSON.h
 *
 * Created: 11/6/2022 6:26:11 PM
 *  Author: dhanu
 */ 


#ifndef CONFIG_JSON_H_
#define CONFIG_JSON_H_

#define ALL_SETTINGS_RELOAD 0
#define CONFIG_RELOAD 1
#define CONTACTS_RELOAD 2
#define ZONES_RELOAD 3
#include <ArduinoJson.h>
#include "statments.h"
#include "typex.h"
#include "TimerSW.h"
#include "FS.h"
#include <SPIFFS.h>
#include "pinsx.h"

	 
void configLoad(uint8_t mode);
bool setJson_key_bool(const char* path, const char* jkey, bool state);
bool getJson_key_char(const char* path, const char* jkey, char*buffer, uint32_t size);	 
bool getJson_key_int(const char* path, const char* jkey, uint32_t *number);
void configReset();
void load_zones(const char* path, uint8_t index_start, uint8_t index_end);

void configSave();
#endif /* CONFIG_JSON_H_ */
