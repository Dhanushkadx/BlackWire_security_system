#pragma once

#include <Arduino.h>
#include <FS.h>
#include "ZoneStorage.h"
#include "zone_engine.h"
#include <typex.h>

class ZoneManager {
public:
    static constexpr uint8_t MAX_ZONES = ZONE_COUNT;  // 48 compile-time

    // ---- Bit positions (match your enums) ----
    enum DeviceTypeBit : uint8_t {
        DT_24H = 0,
        DT_RF,
        DT_SILENT,
        DT_PERIMETER,
        DT_CHIME,
    };

    enum DeviceStateBit : uint8_t {
        DS_EXIT_DELAY = 0,
        DS_ENTRY_DELAY,
        DS_ENABLE,
        DS_BYPASSED,
        DS_ALARM,
        DS_LAST_STATE,   // runtime only
        DS_AVAILABLE
    };

    bool begin(fs::FS& fs, const char* path);

    // True if zones.bin was missing or had a CRC error and was reinitialized with defaults.
    // Check after begin() — if true, cfgIndex zone attr versions should be cleared so
    // MQTT re-fetches zone names on next connect.
    bool wasReinitialized() const { return _wasReinitialized; }

    // Sync persistent flags -> ZoneEngine
    void syncToEngine(ZoneEngine& eng) const;

    // Access raw array (for ZoneEngine Option B if needed)
    MY_SENS*       zones()       { return _zones; }
    const MY_SENS* zones() const { return _zones; }

    // --------- Attribute Getters / Setters ----------
    bool getStateBit(uint8_t z, DeviceStateBit bit) const;
    bool setStateBit(uint8_t z, DeviceStateBit bit, bool val, bool saveNow = true);

    bool getTypeBit(uint8_t z, DeviceTypeBit bit) const;
    bool setTypeBit(uint8_t z, DeviceTypeBit bit, bool val, bool saveNow = true);

    // Convenience
    bool isBypassed(uint8_t z) const { return getStateBit(z, DS_BYPASSED); }
    bool setBypassed(uint8_t z, bool v, bool saveNow = true) { return setStateBit(z, DS_BYPASSED, v, saveNow); }

    bool is24h(uint8_t z) const { return getTypeBit(z, DT_24H); }
    bool set24h(uint8_t z, bool v, bool saveNow = true) { return setTypeBit(z, DT_24H, v, saveNow); }

    // --------- Name API ----------
    bool getName(uint8_t z, char* out, size_t outSize) const;
    bool setName(uint8_t z, const char* name);

    // --------- Device Type Getters ----------
    bool isRF(uint8_t z) const;
    bool isSilent(uint8_t z) const;
    bool isPerimeter(uint8_t z) const;
    bool isEntryDelay(uint8_t z) const;
    bool isExitDelay(uint8_t z) const;
    bool isReady(uint8_t z) const;
    bool isAvailable(uint8_t z) const { return getStateBit(z, DS_AVAILABLE); }
    

    // --------- Device Type Setters ----------
    bool setRF(uint8_t z, bool v, bool saveNow = true);
    bool setSilent(uint8_t z, bool v, bool saveNow = true);
    bool setPerimeter(uint8_t z, bool v, bool saveNow = true);

    bool setExitDelay(uint8_t z, bool v, bool saveNow = true);
    bool setEntryDelay(uint8_t z, bool v, bool saveNow = true);
    bool isChime(uint8_t z) const { return getTypeBit(z, DT_CHIME); }
    bool setChime(uint8_t z, bool v, bool saveNow = true) { return setTypeBit(z, DT_CHIME, v, saveNow); }

    // Print all zone names to Serial (called at boot to verify zones.bin content)
    void printNames() const;

    // Save explicitly (preserves existing names from file)
    bool save();

    // Single atomic write: updates attributes from _zones[], preserves existing file names,
    // but overrides names for zones [base..base+blockSize-1] where hasName[i] is true.
    bool saveWithBlockNames(uint8_t base, const char (*blockNames)[ZONE_NAME_LEN],
                            const bool* hasName, uint8_t blockSize);

private:
    void clearRuntimeBits();
    static uint8_t mask(uint8_t bit) { return (1u << bit); }

private:
    fs::FS* _fs = nullptr;
    const char* _path = nullptr;
    bool _wasReinitialized = false;

    MY_SENS _zones[MAX_ZONES];   // STATIC 48 ARRAY
};

extern ZoneManager gZoneManager;