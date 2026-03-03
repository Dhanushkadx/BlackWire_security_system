#pragma once
/*
    RemoteStorage.h (BaseCode + Slots)
    ---------------------------------
    Human-readable + well-commented remote storage for your alarm panel.

    ✅ What we store (per slot)
       - baseCode : uint32_t  (remote identity, independent of old/new remote type)
       - enabled  : uint8_t   (0/1)

    ✅ What we do NOT store
       - We do NOT store "old/new" remote type
       - We do NOT store 4 buttons separately
       - We do NOT store userId

    ✅ How it works (runtime)
       When an RF code arrives (as decimal string):
         1) Detect whether it matches a known 4-bit command or 8-bit command.
         2) Extract baseCode by shifting out command bits:
              - 4-bit command  -> baseCode = code >> 4
              - 8-bit command  -> baseCode = code >> 8
         3) Find baseCode in the DB (8 slots).
         4) If slot is enabled -> return button number 1..4.

    ✅ Learn (slot-based)
       - You choose the slot number (0..7).
       - learnFromCodeStr(slot, codeStr) stores ONLY baseCode into that slot.
       - Enabling/disabling is done with a separate function setEnabled(slot,...).

    ✅ SPIFFS file format (binary)
       - Header + 8 records + CRC16-CCITT
       - Atomic write using temp file + rename (power-loss safer)

    Button mapping (your remcode() mapping)
       button 1 => 0x01 (4-bit) OR 0xC0 (8-bit)
       button 2 => 0x02 (4-bit) OR 0x03 (8-bit)
       button 3 => 0x04 (4-bit) OR 0x0C (8-bit)
       button 4 => 0x08 (4-bit) OR 0x30 (8-bit)
*/

#include <Arduino.h>
#include <FS.h>

class RemoteStorage {
public:
    // Number of remote slots
    static constexpr uint8_t  REM_MAX = 8;

    // Magic identifies the file format/version.
    // If you change record layout later, change this so firmware can auto-reset file safely.
    static constexpr uint16_t FILE_MAGIC = 0xB44D;

    // Default SPIFFS path
    static constexpr const char* DEFAULT_PATH = "/remotes.bin";

    // -------------------------
    // File layout structs
    // -------------------------
    #pragma pack(push, 1)

    // File header stored at the beginning
    struct FileHeader {
        uint16_t magic;     // FILE_MAGIC
        uint8_t  count;     // REM_MAX
        uint8_t  reserved;  // keep 0 for now
    };

    // One slot (one remote)
    struct RemoteRec {
        uint32_t baseCode;    // 0 = empty slot
        uint8_t  enabled;     // 0/1
        uint8_t  reserved[3]; // future use / alignment
    };

    #pragma pack(pop)

public:
    // -------------------------
    // Boot / persistence
    // -------------------------

    // Call after SPIFFS.begin(true).
    // Loads file into RAM; if missing/corrupt -> creates defaults and writes file.
    static bool begin(fs::FS& fs, const char* path = DEFAULT_PATH);

    // Save current RAM to SPIFFS (atomic write + CRC).
    static bool save();

    // Clear all slots in RAM (and optionally save).
    static bool clearAll(bool saveNow = true);

    // -------------------------
    // Slot-based API (your requirement)
    // -------------------------

    // Learn remote into a specific slot using ANY received button code.
    // - Stores ONLY baseCode into that slot.
    // - Does NOT enable/disable. (enable is separate)
    static bool learnFromCodeStr(uint8_t slot, const char* codeStr, bool saveNow = true);

    // Enable/disable a slot.
    static bool setEnabled(uint8_t slot, bool enable, bool saveNow = true);

    // Remove a slot (make it empty + disabled).
    static bool removeSlot(uint8_t slot, bool saveNow = true);

    // -------------------------
    // Runtime check (when RF code arrives)
    // -------------------------

    // Returns:
    //   0    => not allowed / unknown / invalid
    //   1..4 => button number
    static uint8_t checkAllowedButtonFromCodeStr(const char* codeStr, uint8_t* outSlot = nullptr);

    // -------------------------
    // Debug / UI access
    // -------------------------
    static const RemoteRec* data();   // pointer to internal array
    static uint8_t count();          // always REM_MAX
    static void printToSerial();

    // -------------------------
    // Decode helpers
    // -------------------------

    // Extract baseCode + cmd from received code string.
    // Returns button number 1..4 (or 0 if invalid).
    static uint8_t extractBaseAndCmd(const char* codeStr, uint32_t* outBaseCode, uint8_t* outCmd);
    static uint32_t getBaseCode(uint8_t slot);



private:
    static fs::FS* _fs;
    static const char* _path;
    static RemoteRec _rem[REM_MAX];

    // Storage backend
    static bool loadOrInit();
    static bool validateAndLoad();
    static bool saveToPath(const char* path);

    // Slot helper
    static bool validSlot(uint8_t slot) { return slot < REM_MAX; }

    // Find slot by baseCode
    static int findByBase(uint32_t baseCode);

    // CRC16-CCITT (poly 0x1021), init 0xFFFF
    static uint16_t crc16_update(uint16_t crc, const uint8_t* data, size_t len);
};
