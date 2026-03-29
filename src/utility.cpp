#include "utility.h"

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
    mqtt_publish_boot_snapshot();
}

// ---------------------- Publish Network Info ----------------------
void publish_network_info()
{
    mqtt_publish_latest_attributes();
}



void publish_health_info(float batteryVoltage, uint32_t restartCount)
{
    (void)batteryVoltage;
    (void)restartCount;
    mqtt_publish_telemetry();
}


