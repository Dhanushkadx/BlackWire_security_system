#include "RemoteStorage.h"

// -------------------------
// Static members (internal RAM database)
// -------------------------
fs::FS* RemoteStorage::_fs = nullptr;
const char* RemoteStorage::_path = RemoteStorage::DEFAULT_PATH;
RemoteStorage::RemoteRec RemoteStorage::_rem[RemoteStorage::REM_MAX] = {};

// --------------------------------------------------
// CRC16-CCITT
//   - Polynomial: 0x1021
//   - Init: 0xFFFF
// --------------------------------------------------
uint16_t RemoteStorage::crc16_update(uint16_t crc, const uint8_t* data, size_t len)
{
    while (len--) {
        crc ^= (uint16_t)(*data++) << 8;
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else             crc <<= 1;
        }
    }
    return crc;
}

// --------------------------------------------------
// Clear RAM to defaults
// --------------------------------------------------
bool RemoteStorage::clearAll(bool saveNow)
{
    for (uint8_t i = 0; i < REM_MAX; i++) {
        _rem[i].baseCode = 0; // empty
        _rem[i].enabled  = 0; // disabled
        _rem[i].reserved[0] = 0;
        _rem[i].reserved[1] = 0;
        _rem[i].reserved[2] = 0;
    }
    return saveNow ? save() : true;
}

// --------------------------------------------------
// begin(): connect FS + load remotes
// --------------------------------------------------
bool RemoteStorage::begin(fs::FS& fs, const char* path)
{
    _fs = &fs;
    _path = (path && *path) ? path : DEFAULT_PATH;
    return loadOrInit();
}

// --------------------------------------------------
// Load existing file or create new one
// --------------------------------------------------
bool RemoteStorage::loadOrInit()
{
    if (!_fs) return false;

    if (_fs->exists(_path)) {
        if (validateAndLoad()) return true;
        Serial.println(F("RemoteStorage: file invalid/corrupt -> reset defaults"));
    }

    clearAll(false);
    return save(); // create file
}

// --------------------------------------------------
// Find slot by baseCode
// --------------------------------------------------
int RemoteStorage::findByBase(uint32_t baseCode)
{
    if (baseCode == 0) return -1;
    for (uint8_t i = 0; i < REM_MAX; i++) {
        if (_rem[i].baseCode == baseCode) return (int)i;
    }
    return -1;
}

// --------------------------------------------------
// validateAndLoad(): header + records + CRC check
// --------------------------------------------------
bool RemoteStorage::validateAndLoad()
{
    if (!_fs) return false;

    File f = _fs->open(_path, FILE_READ);
    if (!f) return false;

    const size_t need = sizeof(FileHeader) + sizeof(RemoteRec) * REM_MAX + sizeof(uint16_t);
    if ((size_t)f.size() != need) { f.close(); return false; }

    // Read header
    FileHeader hdr{};
    if (f.readBytes((char*)&hdr, sizeof(hdr)) != (int)sizeof(hdr)) { f.close(); return false; }
    if (hdr.magic != FILE_MAGIC || hdr.count != REM_MAX) { f.close(); return false; }

    // Read records into RAM
    const size_t recBytes = sizeof(RemoteRec) * REM_MAX;
    if (f.readBytes((char*)_rem, recBytes) != (int)recBytes) { f.close(); return false; }

    // CRC check over header+records (exclude stored CRC)
    f.seek(0, SeekSet);
    uint16_t crc = 0xFFFF;

    size_t remaining = need - sizeof(uint16_t);
    uint8_t buf[128];

    while (remaining) {
        size_t chunk = remaining > sizeof(buf) ? sizeof(buf) : remaining;
        int r = f.readBytes((char*)buf, chunk);
        if (r != (int)chunk) { f.close(); return false; }
        crc = crc16_update(crc, buf, chunk);
        remaining -= chunk;
    }

    uint16_t stored = 0;
    if (f.readBytes((char*)&stored, sizeof(stored)) != (int)sizeof(stored)) { f.close(); return false; }
    f.close();

    if (crc != stored) {
        Serial.println(F("RemoteStorage: CRC mismatch"));
        return false;
    }

    return true;
}

// --------------------------------------------------
// save(): atomic write
// --------------------------------------------------
bool RemoteStorage::save()
{
    if (!_fs) return false;
    return saveToPath(_path);
}

bool RemoteStorage::saveToPath(const char* path)
{
    if (!_fs || !path || !*path) return false;

    // Build temp path
    String tmp(path);
    int dot = tmp.lastIndexOf('.');
    if (dot >= 0) tmp = tmp.substring(0, dot) + ".tmp";
    else          tmp += ".tmp";

    if (_fs->exists(tmp)) _fs->remove(tmp);

    // 1) Write header + records to temp file
    File out = _fs->open(tmp, FILE_WRITE);
    if (!out) return false;

    FileHeader hdr{};
    hdr.magic = FILE_MAGIC;
    hdr.count = REM_MAX;
    hdr.reserved = 0;

    if (out.write((const uint8_t*)&hdr, sizeof(hdr)) != sizeof(hdr)) { out.close(); _fs->remove(tmp); return false; }

    const size_t recBytes = sizeof(RemoteRec) * REM_MAX;
    if (out.write((const uint8_t*)_rem, recBytes) != recBytes) { out.close(); _fs->remove(tmp); return false; }

    out.close();

    // 2) Compute CRC over header+records
    File in = _fs->open(tmp, FILE_READ);
    if (!in) { _fs->remove(tmp); return false; }

    uint16_t crc = 0xFFFF;
    size_t remaining = sizeof(FileHeader) + recBytes;
    uint8_t buf[128];

    while (remaining) {
        size_t chunk = remaining > sizeof(buf) ? sizeof(buf) : remaining;
        int r = in.readBytes((char*)buf, chunk);
        if (r != (int)chunk) { in.close(); _fs->remove(tmp); return false; }
        crc = crc16_update(crc, buf, chunk);
        remaining -= chunk;
    }
    in.close();

    // 3) Append CRC to temp file
    File app = _fs->open(tmp, FILE_APPEND);
    if (!app) { _fs->remove(tmp); return false; }
    if (app.write((const uint8_t*)&crc, sizeof(crc)) != sizeof(crc)) { app.close(); _fs->remove(tmp); return false; }
    app.close();

    // 4) Replace original file
    if (_fs->exists(path)) _fs->remove(path);
    return _fs->rename(tmp, path);
}

// --------------------------------------------------
// Slot-based learn: store ONLY baseCode in the slot
// --------------------------------------------------
bool RemoteStorage::learnFromCodeStr(uint8_t slot, const char* codeStr, bool saveNow)
{
    if (!validSlot(slot)) return false;

    uint32_t base = 0;
    uint8_t cmd = 0;

    // This validates codeStr and extracts baseCode
    uint8_t btn = extractBaseAndCmd(codeStr, &base, &cmd);
    (void)cmd; // not needed for learning
    if (btn == 0 || base == 0) return false;

    _rem[slot].baseCode = base;
    _rem[slot].enabled  = 1;

    return saveNow ? save() : true;
}

// --------------------------------------------------
// Slot-based enable/disable
// --------------------------------------------------
bool RemoteStorage::setEnabled(uint8_t slot, bool enable, bool saveNow)
{
    if (!validSlot(slot)) return false;

    _rem[slot].enabled = enable ? 1 : 0;

    return saveNow ? save() : true;
}

// --------------------------------------------------
// Remove slot (empty + disabled)
// --------------------------------------------------
bool RemoteStorage::removeSlot(uint8_t slot, bool saveNow)
{
    if (!validSlot(slot)) return false;

    _rem[slot].baseCode = 0;
    _rem[slot].enabled  = 0;
    _rem[slot].reserved[0] = 0;
    _rem[slot].reserved[1] = 0;
    _rem[slot].reserved[2] = 0;

    return saveNow ? save() : true;
}

// --------------------------------------------------
// Runtime check: find baseCode and return button 1..4
// --------------------------------------------------
uint8_t RemoteStorage::checkAllowedButtonFromCodeStr(const char* codeStr, uint8_t* outSlot)
{
    uint32_t base = 0;
    uint8_t cmd = 0;
    uint8_t btn = extractBaseAndCmd(codeStr, &base, &cmd);
    (void)cmd;

    if (btn == 0 || base == 0) return 0;

    int idx = findByBase(base);
    if (idx < 0) return 0;

    if (_rem[idx].enabled == 0) return 0;

    if (outSlot) *outSlot = (uint8_t)idx;
    return btn;
}

// --------------------------------------------------
// Debug / UI access
// --------------------------------------------------
const RemoteStorage::RemoteRec* RemoteStorage::data() { return _rem; }
uint8_t RemoteStorage::count() { return REM_MAX; }

void RemoteStorage::printToSerial()
{
    Serial.println(F("=== RemoteStorage (slots) ==="));
    for (uint8_t i = 0; i < REM_MAX; i++) {
        Serial.printf_P(PSTR("[%u] base=%lu enabled=%u\n"),
                        (unsigned)i,
                        (unsigned long)_rem[i].baseCode,
                        (unsigned)_rem[i].enabled);
    }
}

// --------------------------------------------------
// extractBaseAndCmd(): your core decoding logic
// --------------------------------------------------
uint8_t RemoteStorage::extractBaseAndCmd(const char* codeStr, uint32_t* outBaseCode, uint8_t* outCmd)
{
    if (outBaseCode) *outBaseCode = 0;
    if (outCmd)      *outCmd = 0;
    if (!codeStr || !*codeStr) return 0;

    unsigned long code = strtoul(codeStr, nullptr, 10);

    uint8_t split4 = (uint8_t)(code & 0x0F);
    uint8_t split8 = (uint8_t)(code & 0xFF);

    uint8_t cmd = 0;
    uint32_t base = 0;

    // 4-bit commands
    if (split4 == 0x01 || split4 == 0x02 || split4 == 0x04 || split4 == 0x08) {
        cmd = split4;
        base = (uint32_t)(code >> 4);
    }
    // 8-bit commands
    else if (split8 == 0xC0 || split8 == 0x03 || split8 == 0x0C || split8 == 0x30) {
        cmd = split8;
        base = (uint32_t)(code >> 8);
    }
    else {
        return 0; // unknown
    }

    // cmd -> button 1..4
    uint8_t btn = 0;
    switch (cmd) {
        case 0x01: case 0xC0: btn = 1; break;
        case 0x02: case 0x03: btn = 2; break;
        case 0x04: case 0x0C: btn = 3; break;
        case 0x08: case 0x30: btn = 4; break;
        default: btn = 0; break;
    }

    if (btn == 0) return 0;

    if (outBaseCode) *outBaseCode = base;
    if (outCmd)      *outCmd = cmd;
    return btn;
}

uint32_t RemoteStorage::getBaseCode(uint8_t slot)
{
    if (slot >= REM_MAX) return 0;   // invalid slot

    return _rem[slot].baseCode;      // may be 0 if empty
}
