#include "utility.h"

#define FIRMWARE_VERSION "1.0.0"
#define HARDWARE_VERSION "1.0.0"
// Globals
uint8_t zoneState[6] = {0};
bool systemArmed = false;

// ---------------------- Zone Functions ----------------------
void setZone(uint8_t zone, bool active)
{
    if(zone >= NUM_ZONES) return;

    uint8_t byteIndex = zone / 8;
    uint8_t bitIndex  = zone % 8;

    if (active)
        zoneState[byteIndex] |= (1 << bitIndex);
    else
        zoneState[byteIndex] &= ~(1 << bitIndex);
}

void zonesToHex(char* output)
{
    for (int i = 0; i < 6; i++)
    {
        sprintf(&output[i * 2], "%02X", zoneState[i]);
    }
    output[12] = '\0';
}

// ---------------------- Publish System State ----------------------
void publish_system_startup_msg()
{
    StaticJsonDocument<256> doc;

    doc["ip"] = WiFi.localIP().toString();
    doc["rssi"] = WiFi.RSSI();
    doc["fw"] = FIRMWARE_VERSION;
    doc["hw"] = HARDWARE_VERSION;
    doc["state"] = systemArmed ? "armed" : "disarmed";
    doc["uptime"] = millis() / 1000;
    doc["gsm"] = "ok";  // Example additional info
    doc["vac"] = "ok";     // Example additional info
    doc["sens"] = 0;        // Example additional info
    

    char zonesHex[13];
    zonesToHex(zonesHex);
    doc["zones"] = zonesHex;

    char payload[256];
    serializeJson(doc, payload);

    //mqtt.publish(TB_ATTR_TOPIC, payload, true);  // mqtt = your transport layer instance
    publish_system_state(payload, "boot", true);

#ifdef _DEBUG
    Serial.println(F("Published system state:"));
    Serial.println(payload);
#endif
}

// ---------------------- Publish Network Info ----------------------
void publish_network_info()
{
    StaticJsonDocument<256> doc;

    // Wi-Fi info
    doc["ssid"]    = WiFi.SSID();
    doc["bssid"]   = WiFi.BSSIDstr();        // MAC of AP
    doc["ip"]      = WiFi.localIP().toString();
    doc["rssi"]    = WiFi.RSSI();
    doc["gateway"] = WiFi.gatewayIP().toString();
    doc["subnet"]  = WiFi.subnetMask().toString();
    doc["mac"]     = WiFi.macAddress();
    doc["gsm_rssi"] = getSignal_strength();
    
    char buff[20];
    if(getGsmOperator(buff, sizeof(buff))){ 
        doc["gsm_op"] = buff;
    } else {
        doc["gsm_op"] = "unknown";
    }           
    char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!
    getEMIE(imei, sizeof(imei));
    doc["EMIE"] = imei;

    // Optional: device firmware version for reference
    doc["fw"] = FIRMWARE_VERSION;
    doc["hw"] = HARDWARE_VERSION;

    char payload[256];
    serializeJson(doc, payload);
    publish_system_state(payload, "network", true);

#ifdef _DEBUG
    Serial.println(F("Published network info to TB attributes:"));
    Serial.println(payload);
#endif
}



void publish_health_info(float batteryVoltage, uint32_t restartCount)
{
    StaticJsonDocument<128> doc;

    doc["uptime_sec"]     = millis() / 1000;
    doc["battery"]        = batteryVoltage;
    doc["restart_count"]  = restartCount;

    char payload[128];
    serializeJson(doc, payload);

    publish_system_state(payload, "health", true);

#ifdef _DEBUG
    Serial.println(F("Published health info to TB attributes:"));
    Serial.println(payload);
#endif
}


