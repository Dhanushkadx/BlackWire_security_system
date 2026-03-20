#include "ZoneManager.h"

ZoneManager gZoneManager;   // GLOBAL SINGLE OBJECT

bool ZoneManager::begin(fs::FS& fs, const char* path)
{
    _fs = &fs;
    _path = path;

    bool ok = ZoneStorage::loadOrInit(*_fs, _path, _zones, MAX_ZONES);
    clearRuntimeBits();
    return ok;
}

void ZoneManager::clearRuntimeBits()
{
    for(uint8_t i = 0; i < MAX_ZONES; i++)
    {
        _zones[i].device_state &= ~mask(DS_LAST_STATE);

        // Recommended (safer)
        _zones[i].device_state &= ~mask(DS_ALARM);
        _zones[i].device_state &= ~mask(DS_AVAILABLE);
        _zones[i].device_state &= ~mask(DS_ENABLE);
    }
}

bool ZoneManager::getStateBit(uint8_t z, DeviceStateBit bit) const
{
    if(z >= MAX_ZONES) return false;
    return (_zones[z].device_state & mask(bit)) != 0;
}

bool ZoneManager::setStateBit(uint8_t z, DeviceStateBit bit, bool val, bool saveNow)
{
    if(z >= MAX_ZONES) return false;

    if(val) _zones[z].device_state |= mask(bit);
    else    _zones[z].device_state &= ~mask(bit);

    if(saveNow) return save();
    return true;
}

bool ZoneManager::getTypeBit(uint8_t z, DeviceTypeBit bit) const
{
    if(z >= MAX_ZONES) return false;
    return (_zones[z].device_type & mask(bit)) != 0;
}

bool ZoneManager::setTypeBit(uint8_t z, DeviceTypeBit bit, bool val, bool saveNow)
{
    if(z >= MAX_ZONES) return false;

    if(val) _zones[z].device_type |= mask(bit);
    else    _zones[z].device_type &= ~mask(bit);

    if(saveNow) return save();
    return true;
}

bool ZoneManager::getName(uint8_t z, char* out, size_t outSize) const
{
    if(z >= MAX_ZONES) return false;
    return ZoneStorage::getName(*_fs, _path, z, out, outSize, MAX_ZONES);
}

bool ZoneManager::setName(uint8_t z, const char* name)
{
    if(z >= MAX_ZONES) return false;
    return ZoneStorage::setName(*_fs, _path, z, name, _zones, MAX_ZONES);
}

bool ZoneManager::save()
{
    return ZoneStorage::savePreserveNames(*_fs, _path, _zones, MAX_ZONES);
}

void ZoneManager::syncToEngine(ZoneEngine& eng) const
{
    for(uint8_t i = 0; i < MAX_ZONES; i++)
    {
        ZoneConfig cfg;
        cfg.bypass = (_zones[i].device_state & mask(DS_BYPASSED)) != 0;
        cfg.is24h  = (_zones[i].device_type  & mask(DT_24H)) != 0;
        cfg.chime  = (_zones[i].device_type  & mask(DT_PERIMETER)) != 0;
        cfg.debounce_ms = 50;
        cfg.momentary_hold_ms = (_zones[i].device_type & mask(DT_RF)) ? 200 : 0;

        eng.setConfig(i, cfg);
    }
}

// -------- GETTERS --------


bool ZoneManager::isRF(uint8_t z) const
{
    return getTypeBit(z, DT_RF);
}

bool ZoneManager::isSilent(uint8_t z) const
{
    return getTypeBit(z, DT_SILENT);
}

bool ZoneManager::isPerimeter(uint8_t z) const
{
    return getTypeBit(z, DT_PERIMETER);
}




bool ZoneManager::isExitDelay(uint8_t z) const
{
    return getStateBit(z, DS_EXIT_DELAY);
}

bool ZoneManager::isEntryDelay(uint8_t z) const
{
    return getStateBit(z, DS_ENTRY_DELAY);
}


bool ZoneManager::isReady(uint8_t z) const
{
    if (z >= MAX_ZONES) return false;

    const auto& zone = _zones[z];

    // RF zones are always considered ready
    if (zone.device_type & mask(DT_RF)) {
        return true;
    }

    bool isOpen   = zone.device_state & mask(DS_LAST_STATE);
    bool bypassed = zone.device_state & mask(DS_BYPASSED);

    // If zone is open and not bypassed → system not ready
    if (isOpen && !bypassed) {
        return false;
    }

    return true;
}


// -------- SETTERS --------


bool ZoneManager::setRF(uint8_t z, bool v, bool saveNow)
{
    return setTypeBit(z, DT_RF, v, saveNow);
}

bool ZoneManager::setSilent(uint8_t z, bool v, bool saveNow)
{
    return setTypeBit(z, DT_SILENT, v, saveNow);
}

bool ZoneManager::setPerimeter(uint8_t z, bool v, bool saveNow)
{
    return setTypeBit(z, DT_PERIMETER, v, saveNow);
}



bool ZoneManager::setExitDelay(uint8_t z, bool v, bool saveNow)
{
    return setStateBit(z, DS_EXIT_DELAY, v, saveNow);
}

bool ZoneManager::setEntryDelay(uint8_t z, bool v, bool saveNow)
{
    return setStateBit(z, DS_ENTRY_DELAY, v, saveNow);
}



