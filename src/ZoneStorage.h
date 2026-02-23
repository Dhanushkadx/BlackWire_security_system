#pragma once

#include <Arduino.h>
#include <FS.h>
#include <typex.h>

// =====================
// Config
// =====================
#ifndef ZONE_COUNT
#define ZONE_COUNT 48
#endif

// =====================
// Bit masks (example)
// =====================
#define BIT_MASK_ENTRY_DELAY   0
#define BIT_MASK_EXIT_DELAY    1
#define BIT_MASK_BYPASSED      2

#define BIT_MASK_RF            0
#define BIT_MASK_PERIMETER     1
#define BIT_MASK_24H           2
#define BIT_MASK_SILENT        3

// =====================
// Structures
// =====================
#pragma pack(push,1)
// typedef struct sensorAtribute {
//     uint8_t device_state;             // bitfield for entry/exit/bypass
//     uint8_t device_type;              // bitfield for RF, perimeter, 24H, silent
//     uint8_t device_connected_hw_address; // sensor HW address
//     long last_updated_time_stamp;     // timestamp
// } MY_SENS, *pMY_SENS;

struct ZoneFileHeader {
    uint16_t magic;      // file signature
    uint8_t version;     // config version
    uint8_t count;       // number of zones
};
#pragma pack(pop)

// =====================
// Constants
// =====================
static const uint16_t ZONE_FILE_MAGIC = 0xA55A;
static const uint8_t  ZONE_FILE_VERSION = 1;

// =====================
// API
// =====================
class ZoneStorage {
public:
    static bool load(fs::FS &fs, const char* path, MY_SENS* zones, uint8_t count);
    static bool save(fs::FS &fs, const char* path, const MY_SENS* zones, uint8_t count);
    static void setDefaults(MY_SENS* zones, uint8_t count);
    static void printZones(const MY_SENS* zones, uint8_t count);
    static void initExampleArray(MY_SENS* zones, uint8_t count);

private:
    static uint16_t crc16_ccitt(const uint8_t* data, size_t length);
};