
//gard band header
#ifndef OTA_H
#define OTA_H

#include "Arduino.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>




String fetchLatestVersion();


bool startOTAUpdate(WiFiClient* client, int contentLength);

void downloadAndApplyFirmware(const char* firmwareUrl) ;
void checkForFirmwareUpdate() ;

void connectToWiFi();

bool startSPIFFSOTAUpdate(WiFiClient* client, int contentLength);
void downloadAndApplySPIFFS(const char* spiffsUrl);

void setup_http();
#endif