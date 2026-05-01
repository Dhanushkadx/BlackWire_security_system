#include "ZoneStorage.h"

// =========================================================
// CRC16-CCITT
// =========================================================
uint16_t ZoneStorage::crc16_ccitt_update(uint16_t crc, const uint8_t* data, size_t length)
{
  while (length--) {
    crc ^= (uint16_t)(*data++) << 8;
    for (uint8_t i = 0; i < 8; i++) {
      if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
      else             crc <<= 1;
    }
  }
  return crc;
}

uint16_t ZoneStorage::crc16_ccitt(const uint8_t* data, size_t length)
{
  return crc16_ccitt_update(0xFFFF, data, length);
}

// =========================================================
// File size / offsets
// =========================================================
size_t ZoneStorage::zonesOffset() { return sizeof(ZoneFileHeader); }
size_t ZoneStorage::zonesSize(uint8_t count) { return sizeof(MY_SENS) * (size_t)count; }
size_t ZoneStorage::namesOffset(uint8_t count) { return zonesOffset() + zonesSize(count); }
size_t ZoneStorage::oneNameOffset(uint8_t index, uint8_t count) {
  return namesOffset(count) + (size_t)index * (size_t)ZONE_NAME_LEN;
}
size_t ZoneStorage::expectedFileSize(uint8_t count) {
  return sizeof(ZoneFileHeader)
       + zonesSize(count)
       + (size_t)count * (size_t)ZONE_NAME_LEN
       + sizeof(uint16_t); // CRC
}

// =========================================================
// Defaults
// =========================================================
void ZoneStorage::setDefaults(MY_SENS* zones, uint8_t count)
{
  // Clear all runtime attributes
  memset(zones, 0, sizeof(MY_SENS) * (size_t)count);

  // You can add any "factory defaults" here if needed.
  // For example:
  // - First 8 zones not bypassed
  // - RF flags for certain zones, etc.
}

void ZoneStorage::makeDefaultName(uint8_t index, char* outName, size_t outSize)
{
  if (!outName || outSize == 0) return;
  snprintf(outName, outSize, "Z%02u", (unsigned)index);
}

// =========================================================
// Atomic write helper
// =========================================================
String ZoneStorage::makeTmpPath(const char* path)
{
  String p(path ? path : "");
  int dot = p.lastIndexOf('.');
  if (dot < 0) return p + ".tmp";
  return p.substring(0, dot) + ".tmp";
}

bool ZoneStorage::writeAtomic(fs::FS &fs, const char* path, const uint8_t* data, size_t length)
{
  String tmp = makeTmpPath(path);

  if (fs.exists(tmp)) fs.remove(tmp);

  File f = fs.open(tmp, FILE_WRITE);
  if (!f) return false;

  size_t w = f.write(data, length);
  f.close();

  if (w != length) {
    fs.remove(tmp);
    return false;
  }

  if (fs.exists(path)) fs.remove(path);
  return fs.rename(tmp, path);
}

// =========================================================
// Validate + load zones
// =========================================================
bool ZoneStorage::validateAndLoadZones(fs::FS &fs, const char* path, MY_SENS* zones, uint8_t count)
{
  File f = fs.open(path, FILE_READ);
  if (!f) return false;

  if ((size_t)f.size() < expectedFileSize(count)) {
    f.close();
    return false;
  }

  ZoneFileHeader hdr{};
  if (f.readBytes((char*)&hdr, sizeof(hdr)) != (int)sizeof(hdr)) {
    f.close();
    return false;
  }

  if (hdr.magic != ZONE_FILE_MAGIC || hdr.count != count) {
    f.close();
    return false;
  }

  // Read zones block into RAM
  const size_t zBytes = zonesSize(count);
  if (f.readBytes((char*)zones, zBytes) != (int)zBytes) {
    f.close();
    return false;
  }

  // Compute CRC over [header + zones + names]
  // We compute it by re-reading the file to avoid allocating a big buffer.
  f.seek(0, SeekSet);

  uint16_t crc = 0xFFFF;
  const size_t dataLen = expectedFileSize(count) - sizeof(uint16_t); // exclude stored CRC
  size_t remaining = dataLen;

  uint8_t buf[128];
  while (remaining) {
    size_t chunk = remaining > sizeof(buf) ? sizeof(buf) : remaining;
    int r = f.readBytes((char*)buf, chunk);
    if (r != (int)chunk) { f.close(); return false; }
    crc = crc16_ccitt_update(crc, buf, chunk);
    remaining -= chunk;
  }

  // Read stored CRC
  uint16_t stored = 0;
  if (f.readBytes((char*)&stored, sizeof(stored)) != (int)sizeof(stored)) {
    f.close();
    return false;
  }

  f.close();

  if (crc != stored) {
    Serial.println(F("ZoneStorage: CRC mismatch (zones.bin corrupted)"));
    return false;
  }

  return true;
}

// =========================================================
// Public: loadOrInit
// =========================================================
static const char* defaultNameGetter(uint8_t index, void* /*ctx*/)
{
  static char buf[ZONE_NAME_LEN];
  memset(buf, 0, sizeof(buf));
  snprintf(buf, sizeof(buf), "Z%02u", (unsigned)index);
  return buf;
}

bool ZoneStorage::loadOrInit(fs::FS &fs, const char* path, MY_SENS* zones, uint8_t count,
                             bool* outWasCreated)
{
  if (!zones || count == 0) return false;

  // If file exists and is valid, load zones into RAM and return.
  if (fs.exists(path)) {
    if (validateAndLoadZones(fs, path, zones, count)) {
      if (outWasCreated) *outWasCreated = false;
      return true;
    }
    Serial.println(F("ZoneStorage: zones.bin CRC/format invalid — reinitialising"));
  }

  // Missing or invalid -> create new file with defaults
  setDefaults(zones, count);

  if (!saveWithNameGetter(fs, path, zones, count, defaultNameGetter, nullptr)) {
    Serial.println(F("ZoneStorage: failed to create default zones.bin"));
    return false;
  }

  if (outWasCreated) *outWasCreated = true;
  return true;
}

// =========================================================
// Stream helpers (names)
// =========================================================
bool ZoneStorage::streamExistingNameToFile(File& src, File& dst, uint8_t index, uint8_t count)
{
  // src is already opened on the existing zones.bin
  if (!src.seek(oneNameOffset(index, count), SeekSet)) return false;

  uint8_t nameBuf[ZONE_NAME_LEN];
  int r = src.readBytes((char*)nameBuf, ZONE_NAME_LEN);
  if (r != (int)ZONE_NAME_LEN) return false;

  return dst.write(nameBuf, ZONE_NAME_LEN) == (size_t)ZONE_NAME_LEN;
}

bool ZoneStorage::streamDefaultOrProvidedName(File& dst, uint8_t index, ZoneNameGetter getName, void* ctx)
{
  char fixed[ZONE_NAME_LEN];
  memset(fixed, 0, sizeof(fixed));

  const char* n = (getName != nullptr) ? getName(index, ctx) : nullptr;
  if (n && n[0]) strlcpy(fixed, n, sizeof(fixed));
  else makeDefaultName(index, fixed, sizeof(fixed));

  return dst.write((const uint8_t*)fixed, ZONE_NAME_LEN) == (size_t)ZONE_NAME_LEN;
}

// =========================================================
// Public: saveWithNameGetter
// =========================================================
bool ZoneStorage::saveWithNameGetter(fs::FS &fs, const char* path,
                                    const MY_SENS* zones, uint8_t count,
                                    ZoneNameGetter getName, void* ctx)
{
  if (!zones || count == 0) return false;

  const String tmp = makeTmpPath(path);
  if (fs.exists(tmp)) fs.remove(tmp);

  File out = fs.open(tmp, FILE_WRITE);
  if (!out) return false;

  // Write header
  ZoneFileHeader hdr{};
  hdr.magic = ZONE_FILE_MAGIC;
  hdr.count = count;
  hdr.reserved = 0;

  if (out.write((uint8_t*)&hdr, sizeof(hdr)) != sizeof(hdr)) {
    out.close(); fs.remove(tmp); return false;
  }

  // Write zones block
  const size_t zBytes = zonesSize(count);
  if (out.write((const uint8_t*)zones, zBytes) != zBytes) {
    out.close(); fs.remove(tmp); return false;
  }

  // Write names sequentially (no caching)
  for (uint8_t i = 0; i < count; i++) {
    if (!streamDefaultOrProvidedName(out, i, getName, ctx)) {
      out.close(); fs.remove(tmp); return false;
    }
  }

  // Compute CRC over what we wrote so far:
  // We re-open the temp file and compute CRC (streaming, low RAM).
  out.close();

  File in = fs.open(tmp, FILE_READ);
  if (!in) { fs.remove(tmp); return false; }

  const size_t dataLen = sizeof(ZoneFileHeader) + zBytes + (size_t)count * (size_t)ZONE_NAME_LEN;
  uint16_t crc = 0xFFFF;
  size_t remaining = dataLen;

  uint8_t buf[128];
  while (remaining) {
    size_t chunk = remaining > sizeof(buf) ? sizeof(buf) : remaining;
    int r = in.readBytes((char*)buf, chunk);
    if (r != (int)chunk) { in.close(); fs.remove(tmp); return false; }
    crc = crc16_ccitt_update(crc, buf, chunk);
    remaining -= chunk;
  }
  in.close();

  // Append CRC
  File append = fs.open(tmp, FILE_APPEND);
  if (!append) { fs.remove(tmp); return false; }
  if (append.write((uint8_t*)&crc, sizeof(crc)) != sizeof(crc)) {
    append.close(); fs.remove(tmp); return false;
  }
  append.close();

  // Replace destination atomically by rename
  if (fs.exists(path)) fs.remove(path);
  return fs.rename(tmp, path);
}

// =========================================================
// Public: savePreserveNames
// =========================================================
bool ZoneStorage::savePreserveNames(fs::FS &fs, const char* path, const MY_SENS* zones, uint8_t count)
{
  if (!zones || count == 0) return false;

  // If file doesn't exist, create with default names.
  if (!fs.exists(path)) {
    return saveWithNameGetter(fs, path, zones, count, defaultNameGetter, nullptr);
  }

  File src = fs.open(path, FILE_READ);
  if (!src) return false;

  // Validate header quickly (avoid copying corrupted names)
  ZoneFileHeader hdr{};
  if (src.readBytes((char*)&hdr, sizeof(hdr)) != (int)sizeof(hdr)) { src.close(); return false; }
  if (hdr.magic != ZONE_FILE_MAGIC || hdr.count != count) { src.close(); return false; }

  const String tmp = makeTmpPath(path);
  if (fs.exists(tmp)) fs.remove(tmp);

  File out = fs.open(tmp, FILE_WRITE);
  if (!out) { src.close(); return false; }

  // Write new header + new zones
  ZoneFileHeader newHdr{};
  newHdr.magic = ZONE_FILE_MAGIC;
  newHdr.count = count;
  newHdr.reserved = 0;

  if (out.write((uint8_t*)&newHdr, sizeof(newHdr)) != sizeof(newHdr)) {
    out.close(); src.close(); fs.remove(tmp); return false;
  }

  const size_t zBytes = zonesSize(count);
  if (out.write((const uint8_t*)zones, zBytes) != zBytes) {
    out.close(); src.close(); fs.remove(tmp); return false;
  }

  // Copy names from existing file (stream one-by-one)
  for (uint8_t i = 0; i < count; i++) {
    if (!streamExistingNameToFile(src, out, i, count)) {
      out.close(); src.close(); fs.remove(tmp); return false;
    }
  }

  out.close();
  src.close();

  // Compute CRC on tmp and append
  File in = fs.open(tmp, FILE_READ);
  if (!in) { fs.remove(tmp); return false; }

  const size_t dataLen = sizeof(ZoneFileHeader) + zBytes + (size_t)count * (size_t)ZONE_NAME_LEN;
  uint16_t crc = 0xFFFF;
  size_t remaining = dataLen;

  uint8_t buf[128];
  while (remaining) {
    size_t chunk = remaining > sizeof(buf) ? sizeof(buf) : remaining;
    int r = in.readBytes((char*)buf, chunk);
    if (r != (int)chunk) { in.close(); fs.remove(tmp); return false; }
    crc = crc16_ccitt_update(crc, buf, chunk);
    remaining -= chunk;
  }
  in.close();

  File append = fs.open(tmp, FILE_APPEND);
  if (!append) { fs.remove(tmp); return false; }
  if (append.write((uint8_t*)&crc, sizeof(crc)) != sizeof(crc)) {
    append.close(); fs.remove(tmp); return false;
  }
  append.close();

  if (fs.exists(path)) fs.remove(path);
  return fs.rename(tmp, path);
}

// =========================================================
// Public: savePreserveNamesWithOverride
// =========================================================
bool ZoneStorage::savePreserveNamesWithOverride(fs::FS &fs, const char* path,
                                                const MY_SENS* zones, uint8_t count,
                                                uint8_t base, uint8_t blockSize,
                                                const char (*blockNames)[ZONE_NAME_LEN],
                                                const bool* hasName)
{
  if (!zones || count == 0 || !blockNames || !hasName) return false;

  // If file doesn't exist, create with default names first, then fall through to override.
  if (!fs.exists(path)) {
    if (!saveWithNameGetter(fs, path, zones, count, defaultNameGetter, nullptr)) return false;
  }

  File src = fs.open(path, FILE_READ);
  if (!src) return false;

  ZoneFileHeader hdr{};
  if (src.readBytes((char*)&hdr, sizeof(hdr)) != (int)sizeof(hdr)) { src.close(); return false; }
  if (hdr.magic != ZONE_FILE_MAGIC || hdr.count != count) { src.close(); return false; }

  const String tmp = makeTmpPath(path);
  if (fs.exists(tmp)) fs.remove(tmp);

  File out = fs.open(tmp, FILE_WRITE);
  if (!out) { src.close(); return false; }

  ZoneFileHeader newHdr{};
  newHdr.magic = ZONE_FILE_MAGIC;
  newHdr.count = count;
  newHdr.reserved = 0;

  if (out.write((uint8_t*)&newHdr, sizeof(newHdr)) != sizeof(newHdr)) {
    out.close(); src.close(); fs.remove(tmp); return false;
  }

  const size_t zBytes = zonesSize(count);
  if (out.write((const uint8_t*)zones, zBytes) != zBytes) {
    out.close(); src.close(); fs.remove(tmp); return false;
  }

  for (uint8_t i = 0; i < count; i++) {
    const uint8_t rel = (uint8_t)(i - base);
    const bool inBlock = (i >= base) && (rel < blockSize) && hasName[rel];

    if (inBlock) {
      char fixed[ZONE_NAME_LEN];
      memset(fixed, 0, sizeof(fixed));
      strlcpy(fixed, blockNames[rel], sizeof(fixed));
      if (out.write((const uint8_t*)fixed, ZONE_NAME_LEN) != (size_t)ZONE_NAME_LEN) {
        out.close(); src.close(); fs.remove(tmp); return false;
      }
    } else {
      if (!streamExistingNameToFile(src, out, i, count)) {
        out.close(); src.close(); fs.remove(tmp); return false;
      }
    }
  }

  out.close();
  src.close();

  File in = fs.open(tmp, FILE_READ);
  if (!in) { fs.remove(tmp); return false; }

  const size_t dataLen = sizeof(ZoneFileHeader) + zBytes + (size_t)count * (size_t)ZONE_NAME_LEN;
  uint16_t crc = 0xFFFF;
  size_t remaining = dataLen;

  uint8_t buf[128];
  while (remaining) {
    size_t chunk = remaining > sizeof(buf) ? sizeof(buf) : remaining;
    int r = in.readBytes((char*)buf, chunk);
    if (r != (int)chunk) { in.close(); fs.remove(tmp); return false; }
    crc = crc16_ccitt_update(crc, buf, chunk);
    remaining -= chunk;
  }
  in.close();

  File append = fs.open(tmp, FILE_APPEND);
  if (!append) { fs.remove(tmp); return false; }
  if (append.write((uint8_t*)&crc, sizeof(crc)) != sizeof(crc)) {
    append.close(); fs.remove(tmp); return false;
  }
  append.close();

  if (fs.exists(path)) fs.remove(path);
  return fs.rename(tmp, path);
}

// =========================================================
// Public: getName
// =========================================================
bool ZoneStorage::getName(fs::FS &fs, const char* path, uint8_t index,
                          char* outName, size_t outSize, uint8_t count)
{
  if (!outName || outSize == 0) return false;
  outName[0] = '\0';
  if (index >= count) return false;

  File f = fs.open(path, FILE_READ);
  if (!f) return false;

  ZoneFileHeader hdr{};
  if (f.readBytes((char*)&hdr, sizeof(hdr)) != (int)sizeof(hdr)) { f.close(); return false; }
  if (hdr.magic != ZONE_FILE_MAGIC || hdr.count != count) { f.close(); return false; }

  if (!f.seek(oneNameOffset(index, count), SeekSet)) { f.close(); return false; }

  char tmp[ZONE_NAME_LEN];
  int r = f.readBytes(tmp, ZONE_NAME_LEN);
  f.close();

  if (r != (int)ZONE_NAME_LEN) return false;

  tmp[ZONE_NAME_LEN - 1] = '\0';
  strlcpy(outName, tmp, outSize);
  return true;
}

// =========================================================
// Public: setName (atomic rewrite, no name caching)
// =========================================================
struct SetNameCtx {
  uint8_t target;
  const char* newName;
};

static const char* setNameGetter(uint8_t index, void* ctxPtr)
{
  SetNameCtx* ctx = (SetNameCtx*)ctxPtr;
  if (index == ctx->target) return ctx->newName;
  return nullptr; // means: preserve existing/default in our wrapper below
}

bool ZoneStorage::setName(fs::FS &fs, const char* path, uint8_t index,
                          const char* newName, const MY_SENS* zones, uint8_t count)
{
  if (!zones || index >= count) return false;

  // If the file doesn't exist, create it first with defaults.
  if (!fs.exists(path)) {
    // Create file with default names then update target name.
    if (!saveWithNameGetter(fs, path, zones, count, defaultNameGetter, nullptr)) return false;
  }

  // We'll preserve existing names except for the one we want to update.
  // To do that without caching, we copy names from the existing file into the new file,
  // but replace the target index.
  File src = fs.open(path, FILE_READ);
  if (!src) return false;

  ZoneFileHeader hdr{};
  if (src.readBytes((char*)&hdr, sizeof(hdr)) != (int)sizeof(hdr)) { src.close(); return false; }
  if (hdr.magic != ZONE_FILE_MAGIC || hdr.count != count) { src.close(); return false; }

  const String tmp = makeTmpPath(path);
  if (fs.exists(tmp)) fs.remove(tmp);

  File out = fs.open(tmp, FILE_WRITE);
  if (!out) { src.close(); return false; }

  ZoneFileHeader newHdr{};
  newHdr.magic = ZONE_FILE_MAGIC;
  newHdr.count = count;
  newHdr.reserved = 0;

  if (out.write((uint8_t*)&newHdr, sizeof(newHdr)) != sizeof(newHdr)) {
    out.close(); src.close(); fs.remove(tmp); return false;
  }

  const size_t zBytes = zonesSize(count);
  if (out.write((const uint8_t*)zones, zBytes) != zBytes) {
    out.close(); src.close(); fs.remove(tmp); return false;
  }

  for (uint8_t i = 0; i < count; i++) {
    if (i == index) {
      // write new name
      char fixed[ZONE_NAME_LEN];
      memset(fixed, 0, sizeof(fixed));
      if (newName && newName[0]) strlcpy(fixed, newName, sizeof(fixed));
      else makeDefaultName(i, fixed, sizeof(fixed));

      if (out.write((const uint8_t*)fixed, ZONE_NAME_LEN) != (size_t)ZONE_NAME_LEN) {
        out.close(); src.close(); fs.remove(tmp); return false;
      }
    } else {
      // copy existing name
      if (!streamExistingNameToFile(src, out, i, count)) {
        out.close(); src.close(); fs.remove(tmp); return false;
      }
    }
  }

  out.close();
  src.close();

  // Compute CRC and append
  File in = fs.open(tmp, FILE_READ);
  if (!in) { fs.remove(tmp); return false; }

  const size_t dataLen = sizeof(ZoneFileHeader) + zBytes + (size_t)count * (size_t)ZONE_NAME_LEN;
  uint16_t crc = 0xFFFF;
  size_t remaining = dataLen;

  uint8_t buf[128];
  while (remaining) {
    size_t chunk = remaining > sizeof(buf) ? sizeof(buf) : remaining;
    int r = in.readBytes((char*)buf, chunk);
    if (r != (int)chunk) { in.close(); fs.remove(tmp); return false; }
    crc = crc16_ccitt_update(crc, buf, chunk);
    remaining -= chunk;
  }
  in.close();

  File append = fs.open(tmp, FILE_APPEND);
  if (!append) { fs.remove(tmp); return false; }
  if (append.write((uint8_t*)&crc, sizeof(crc)) != sizeof(crc)) {
    append.close(); fs.remove(tmp); return false;
  }
  append.close();

  if (fs.exists(path)) fs.remove(path);
  return fs.rename(tmp, path);
}

// =========================================================
// Debug: printZones
// =========================================================
void ZoneStorage::printZones(fs::FS &fs, const char* path, const MY_SENS* zones, uint8_t count)
{
  Serial.println(F("=== Zone Details ==="));
  for (uint8_t i = 0; i < count; i++) {
    Serial.printf_P(PSTR("Zone %02u\n"), (unsigned)i);

    // Read name on-demand
    char nameBuf[ZONE_NAME_LEN];
    if (getName(fs, path, i, nameBuf, sizeof(nameBuf), count)) {
      Serial.print(F("  name: ")); Serial.println(nameBuf);
    } else {
      Serial.print(F("  name: (unavailable)\n"));
    }

    // device_state
    Serial.print(F("  state: "));
    bool anyState = false;
    if (zones[i].device_state & (1 << BIT_MASK_ENTRY_DELAY)) { Serial.print(F("ENTRY_DELAY ")); anyState = true; }
    if (zones[i].device_state & (1 << BIT_MASK_EXIT_DELAY))  { Serial.print(F("EXIT_DELAY "));  anyState = true; }
    if (zones[i].device_state & (1 << BIT_MASK_BYPASSED))    { Serial.print(F("BYPASSED "));    anyState = true; }
    if (!anyState) Serial.print(F("none"));
    Serial.println();

    // device_type
    Serial.print(F("  type: "));
    bool anyType = false;
    if (zones[i].device_type & (1 << BIT_MASK_RF))        { Serial.print(F("RF "));        anyType = true; }
    if (zones[i].device_type & (1 << BIT_MASK_PERIMETER)) { Serial.print(F("PERIMETER ")); anyType = true; }
    if (zones[i].device_type & (1 << BIT_MASK_24H))       { Serial.print(F("24H "));       anyType = true; }
    if (zones[i].device_type & (1 << BIT_MASK_SILENT))    { Serial.print(F("SILENT "));    anyType = true; }
    if (!anyType) Serial.print(F("none"));
    Serial.println();

    Serial.printf_P(PSTR("  card_id: %u\n"), (unsigned)zones[i].device_card_id);
    Serial.printf_P(PSTR("  last_updated: %ld\n"), zones[i].last_updated_time_stamp);
  }
}
