#pragma once
/*
 * ZoneStorage.h
 *
 * Single-file, binary zone storage for an ESP32 alarm panel.
 *
 * Goals:
 *  - Keep zone runtime attributes in RAM (MY_SENS array).
 *  - Store zone names in SPIFFS without keeping them in RAM.
 *  - One fixed binary format (no V1/V2 migration logic).
 *  - If the file is missing or corrupt, auto-create it with defaults.
 *
 * File layout (zones.bin):
 *  [Header]
 *  [MY_SENS zones[count]]
 *  [char names[count][ZONE_NAME_LEN]]   (fixed-length, null-terminated if shorter)
 *  [uint16_t crc16_ccitt]               (CRC over header + zones + names)
 */

#include <Arduino.h>
#include <FS.h>
#include <typex.h>
#include "event_bus.h"

// ---------------------
// Bit masks
// ---------------------
// device_state bitfield
#define BIT_MASK_ENTRY_DELAY   0
#define BIT_MASK_EXIT_DELAY    1
#define BIT_MASK_BYPASSED      2

// device_type bitfield
#define BIT_MASK_RF            0
#define BIT_MASK_PERIMETER     1
#define BIT_MASK_24H           2
#define BIT_MASK_SILENT        3

// ---------------------
// Storage constants
// ---------------------
static const uint16_t ZONE_FILE_MAGIC = 0xA55A;

// Fixed name storage length per zone.
// Keep it short for SMS and small UIs.
#ifndef ZONE_NAME_LEN
#define ZONE_NAME_LEN 16
#endif

// ---------------------
// Binary header
// ---------------------
#pragma pack(push, 1)
struct ZoneFileHeader {
  uint16_t magic;   // ZONE_FILE_MAGIC
  uint8_t  count;   // number of zones in this file (usually 48)
  uint8_t  reserved; // reserved for future (set to 0)
};
#pragma pack(pop)

// ---------------------
// API
// ---------------------
class ZoneStorage {
public:
  // Load zones into RAM.
  // If zones.bin is missing or invalid, this function creates a new file
  // with default attributes and default names, and returns true.
  // outWasCreated is set to true if the file had to be (re)created, false if loaded normally.
  static bool loadOrInit(fs::FS &fs, const char* path, MY_SENS* zones, uint8_t count,
                         bool* outWasCreated = nullptr);

  // Save zones attributes while preserving existing names stored in the file.
  // (Useful when only flags change and you don't want to provide names.)
  static bool savePreserveNames(fs::FS &fs, const char* path, const MY_SENS* zones, uint8_t count);

  // Save zones attributes + names, overriding names for a block of zones [base..base+blockSize-1].
  // For zones in the block: uses blockNames[rel] if hasName[rel] is true, else preserves file name.
  // For zones outside the block: preserves existing file names.
  // Single atomic write — no per-zone rewrites.
  static bool savePreserveNamesWithOverride(fs::FS &fs, const char* path,
                                            const MY_SENS* zones, uint8_t count,
                                            uint8_t base, uint8_t blockSize,
                                            const char (*blockNames)[ZONE_NAME_LEN],
                                            const bool* hasName);

  // Save zones attributes + names, where names are obtained by a callback.
  // This avoids keeping 48 names in RAM: we stream names one-by-one to the file.
  typedef const char* (*ZoneNameGetter)(uint8_t index, void* ctx);
  static bool saveWithNameGetter(fs::FS &fs, const char* path,
                                const MY_SENS* zones, uint8_t count,
                                ZoneNameGetter getName, void* ctx);

  // Read a single zone name from the file (no caching).
  static bool getName(fs::FS &fs, const char* path, uint8_t index,
                      char* outName, size_t outSize, uint8_t count);

  // Update a single zone name (atomic rewrite; keeps CRC valid).
  // Uses the provided zones array (so we don't have to re-load zones from file).
  static bool setName(fs::FS &fs, const char* path, uint8_t index,
                      const char* newName, const MY_SENS* zones, uint8_t count);

  // Helpers
  static void setDefaults(MY_SENS* zones, uint8_t count);
  static void makeDefaultName(uint8_t index, char* outName, size_t outSize);

  // Debug helper (prints attributes; names are read on-demand)
  static void printZones(fs::FS &fs, const char* path, const MY_SENS* zones, uint8_t count);

private:
  // CRC16-CCITT (0x1021), init 0xFFFF
  static uint16_t crc16_ccitt_update(uint16_t crc, const uint8_t* data, size_t length);
  static uint16_t crc16_ccitt(const uint8_t* data, size_t length);

  // File math
  static size_t expectedFileSize(uint8_t count);
  static size_t zonesOffset();
  static size_t zonesSize(uint8_t count);
  static size_t namesOffset(uint8_t count);
  static size_t oneNameOffset(uint8_t index, uint8_t count);

  // Low-level helpers
  static bool validateAndLoadZones(fs::FS &fs, const char* path, MY_SENS* zones, uint8_t count);
  static bool writeAtomic(fs::FS &fs, const char* path, const uint8_t* data, size_t length);
  static String makeTmpPath(const char* path);

  // Name streaming helpers
  static bool streamExistingNameToFile(File& src, File& dst, uint8_t index, uint8_t count);
  static bool streamDefaultOrProvidedName(File& dst, uint8_t index, ZoneNameGetter getName, void* ctx);
};
