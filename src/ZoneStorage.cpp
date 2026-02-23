#include "ZoneStorage.h"

// =====================
// CRC16 CCITT
// =====================
uint16_t ZoneStorage::crc16_ccitt(const uint8_t* data, size_t length)
{
    uint16_t crc = 0xFFFF;
    while (length--) {
        crc ^= (*data++) << 8;
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

// =====================
// Set defaults
// =====================
void ZoneStorage::setDefaults(MY_SENS* zones, uint8_t count)
{
    memset(zones, 0, sizeof(MY_SENS) * count);
}

// =====================
// Save
// =====================
bool ZoneStorage::save(fs::FS &fs, const char* path, const MY_SENS* zones, uint8_t count)
{
    File f = fs.open(path, FILE_WRITE);
    if (!f) return false;

    ZoneFileHeader hdr;
    hdr.magic = ZONE_FILE_MAGIC;
    hdr.version = ZONE_FILE_VERSION;
    hdr.count = count;

    // write header
    if (f.write((uint8_t*)&hdr, sizeof(hdr)) != sizeof(hdr)) { f.close(); return false; }

    // write zones
    size_t zoneBytes = sizeof(MY_SENS) * count;
    if (f.write((uint8_t*)zones, zoneBytes) != zoneBytes) { f.close(); return false; }

    // compute CRC over header + zones
    uint16_t crc = crc16_ccitt((uint8_t*)&hdr, sizeof(hdr));
    crc ^= crc16_ccitt((uint8_t*)zones, zoneBytes);

    // write CRC
    if (f.write((uint8_t*)&crc, sizeof(crc)) != sizeof(crc)) { f.close(); return false; }

    f.close();
    return true;
}

// =====================
// Load
// =====================
bool ZoneStorage::load(fs::FS &fs, const char* path, MY_SENS* zones, uint8_t count)
{
    File f = fs.open(path, FILE_READ);
    if (!f) return false;

    ZoneFileHeader hdr;

    if (f.read((uint8_t*)&hdr, sizeof(hdr)) != sizeof(hdr)) { f.close(); return false; }

    if (hdr.magic != ZONE_FILE_MAGIC || hdr.version != ZONE_FILE_VERSION || hdr.count != count) {
        f.close();
        return false;
    }

    size_t zoneBytes = sizeof(MY_SENS) * count;
    if (f.read((uint8_t*)zones, zoneBytes) != zoneBytes) { f.close(); return false; }

    uint16_t storedCrc;
    if (f.read((uint8_t*)&storedCrc, sizeof(storedCrc)) != sizeof(storedCrc)) { f.close(); return false; }

    f.close();

    // recompute CRC
    uint16_t calcCrc = crc16_ccitt((uint8_t*)&hdr, sizeof(hdr));
    calcCrc ^= crc16_ccitt((uint8_t*)zones, zoneBytes);

    if (calcCrc != storedCrc) {
        Serial.println(F("Zone CRC FAIL"));
        return false;
    }

    return true;
}

// =====================
// Print human-readable
// =====================
void ZoneStorage::printZones(const MY_SENS* zones, uint8_t count)
{
    Serial.println(F("=== Zone Details ==="));
    for (uint8_t i = 0; i < count; i++) {
        Serial.printf_P(PSTR("Zone) %02d:\n"), i);

        // device_state
        Serial.print(F("  device_state: "));
        bool anyState = false;
        if (zones[i].device_state & (1 << BIT_MASK_ENTRY_DELAY)) { Serial.print(F("ENTRY_DELAY ")); anyState=true; }
        if (zones[i].device_state & (1 << BIT_MASK_EXIT_DELAY))  { Serial.print(F("EXIT_DELAY ")); anyState=true; }
        if (zones[i].device_state & (1 << BIT_MASK_BYPASSED))    { Serial.print(F("BYPASSED ")); anyState=true; }
        if (!anyState) Serial.print(F("none"));
        Serial.println();

        // device_type
        Serial.print(F("  device_type: "));
        bool anyType = false;
        if (zones[i].device_type & (1 << BIT_MASK_RF))        { Serial.print(F("RF ")); anyType=true; }
        if (zones[i].device_type & (1 << BIT_MASK_PERIMETER)) { Serial.print(F("PERIMETER ")); anyType=true; }
        if (zones[i].device_type & (1 << BIT_MASK_24H))       { Serial.print(F("24H ")); anyType=true; }
        if (zones[i].device_type & (1 << BIT_MASK_SILENT))    { Serial.print(F("SILENT ")); anyType=true; }
        if (!anyType) Serial.print(F("none"));
        Serial.println();

        Serial.printf_P(PSTR("  hw_address: %d\n"), zones[i].device_card_id);
        Serial.printf_P(PSTR("  last_updated: %lu\n"), zones[i].last_updated_time_stamp);
    }
}

// =======================
// Example initialization
void ZoneStorage::initExampleArray(MY_SENS* zones, uint8_t count) {

    for (uint8_t i = 0; i < count; i++) {
        // Clear all flags first
        zones[i].device_state = 0;
        zones[i].device_type = 0;
        zones[i].device_card_id= i;  // example: HW address = index
        zones[i].last_updated_time_stamp = millis();

        if (i <= 7) {
            // Zones 0-7 rules
            // device_state: no entry/exit delay, no bypass
            zones[i].device_state = 0;

            // device_type
            if (i >= 4 && i <= 7) {
                zones[i].device_type |= (1 << BIT_MASK_RF);  // RF enabled
            }
            // no silent, no 24H, no perimeter
        } else {
            // Zones 8+ rules
            zones[i].device_state |= (1 << BIT_MASK_BYPASSED); // bypassed
            zones[i].device_state &= ~((1 << BIT_MASK_ENTRY_DELAY) | (1 << BIT_MASK_EXIT_DELAY));

            // device_type: silent, no RF, no perimeter, no 24H
            zones[i].device_type |= (1 << BIT_MASK_SILENT);
            zones[i].device_type &= ~((1 << BIT_MASK_RF) | (1 << BIT_MASK_PERIMETER) | (1 << BIT_MASK_24H));
        }
    }
}
