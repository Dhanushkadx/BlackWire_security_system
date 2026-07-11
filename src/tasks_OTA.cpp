// tasks_OTA.cpp
#include "tasks_OTA.h"

#include <WiFi.h>
#include <SPIFFS.h>
#include <Update.h>
#include <Preferences.h>       // NVS boot-outcome markers
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>    // streaming whole-image hash
#include <string.h>
#include <strings.h>           // strcasecmp
#include <esp_partition.h>     // fs partition capacity for the pre-flight size check
#include "pinsx.h"             // FW_VER

// The MQTT client is the single already-open connection used everywhere else
// in this project (see msg_store.h / mqtt_broker.cpp).
extern PubSubClient client;

// ── Chunked MQTT OTA ───────────────────────────────────────────────────────
// The whole image is pulled in small chunks over the LIVE MQTT connection
// instead of a second HTTPS/TLS session, so the OTA no longer needs a big
// contiguous heap block on a board that also runs GSM. A server-side
// responder answers meta/req and chunk/req for whichever image is currently
// queued. Protocol, under blackwire/<MAC>/...:
//   device -> ota[fs]/meta/req           {"reqId":n}
//   server -> ota[fs]/meta/res           {"version","size","sha256","chunkSize","target"}
//   device -> ota[fs]/chunk/req          {"reqId":n,"offset":o,"len":l}
//   server -> ota[fs]/chunk/res/<offset> RAW BINARY bytes [o..o+l)
//   device -> ota[fs]/state              {"state":...}
// <offset> is in the chunk/res TOPIC (not the payload) so a stale/duplicate
// retried chunk can be rejected cheaply.

// ── Tunables ─────────────────────────────────────────────────────────────
static const uint32_t OTA_META_TIMEOUT_MS  = 8000;
static const uint8_t  OTA_META_RETRIES     = 3;
static const uint32_t OTA_CHUNK_TIMEOUT_MS = 10000;
static const uint8_t  OTA_CHUNK_RETRIES    = 3;
static const uint8_t  OTA_MAX_PAUSES       = 10;
static const uint16_t OTA_CHUNK_BUF_MAX    = 1536;
static const size_t   OTA_TOPIC_LEN        = 64;

// PubSubClient is NOT thread-safe, so the whole OTA (which pumps client.loop()
// and client.publish() in a tight loop) must run on the SAME task that owns the
// MQTT connection — Task8 via mqtt_com_loop()/TasksOTA::service() — never on a
// side task. request() therefore only latches a flag; service() runs the
// session on the next Task8 iteration, after the current callback() returns.
static volatile bool s_run_pending = false;
static bool          s_run_is_fs   = false;

// ── Session state (single OTA at a time; all runs on the MQTT task) ───────
static volatile bool s_active = false;
static volatile bool s_paused = false;
static uint8_t       s_pause_count = 0;

static volatile bool s_meta_ready = false;
static char          s_meta_json[192];

static volatile bool s_chunk_ready = false;
static uint32_t      s_chunk_off = 0;
static uint16_t      s_chunk_len = 0;
static uint8_t       s_chunk_buf[OTA_CHUNK_BUF_MAX];

static char    s_topic_meta_res[OTA_TOPIC_LEN];
static char    s_topic_meta_req[OTA_TOPIC_LEN];
static char    s_topic_chunk_req[OTA_TOPIC_LEN];
static char    s_topic_chunk_res_pfx[OTA_TOPIC_LEN];
static uint8_t s_topic_chunk_res_pfx_len = 0;
static char    s_topic_state[OTA_TOPIC_LEN];
static bool    s_is_fs        = false;
static bool    s_fs_unmounted = false;   // SPIFFS.end() done — a failure past
                                          // this point must reboot to recover

static uint32_t s_size      = 0;
static uint32_t s_chunkSize = 1536;
static char     s_sha_expect[72] = {0};
static char     s_version[24]    = {0};
static uint32_t s_offset    = 0;
static mbedtls_sha256_context s_sha;

// ── Module wiring ───────────────────────────────────────────────────────
static OtaMqttPublishCb sPub = nullptr;
static const char*      sStatusTopic = nullptr;

// ── Boot outcome (persists across the reboot on a completed OTA) ─────────
static bool s_booted_after_ota = false;
static bool s_booted_aborted   = false;
static bool s_boot_was_fs      = false;
static char s_boot_fsver[24]   = {0};

// ── Small helpers ──────────────────────────────────────────────────────
static void device_id_mac(char* out, size_t len) {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(out, len, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void build_topics(bool is_fs) {
    char mac[13];
    device_id_mac(mac, sizeof(mac));
    const char* t = is_fs ? "otafs" : "ota";
    snprintf(s_topic_meta_res,      sizeof(s_topic_meta_res),      "blackwire/%s/%s/meta/res",    mac, t);
    snprintf(s_topic_meta_req,      sizeof(s_topic_meta_req),      "blackwire/%s/%s/meta/req",    mac, t);
    snprintf(s_topic_chunk_req,     sizeof(s_topic_chunk_req),     "blackwire/%s/%s/chunk/req",   mac, t);
    snprintf(s_topic_chunk_res_pfx, sizeof(s_topic_chunk_res_pfx), "blackwire/%s/%s/chunk/res/",  mac, t);
    snprintf(s_topic_state,         sizeof(s_topic_state),         "blackwire/%s/%s/state",       mac, t);
    s_topic_chunk_res_pfx_len = strlen(s_topic_chunk_res_pfx);
}

// Coarse-grained announcements on the existing "info/sys/ota" status topic.
static void publishStatus(const char* msg) {
    if (sPub && sStatusTopic) sPub(sStatusTopic, msg, false);
}

// Fine-grained protocol state on ota[fs]/state, published directly.
static void publish_ota_state(const char* json) {
    client.publish(s_topic_state, json);
}

static void hex32(const uint8_t* d, char* out) {   // 32 bytes -> 64 lowercase hex + NUL
    static const char* h = "0123456789abcdef";
    for (int i = 0; i < 32; i++) { out[i*2] = h[d[i] >> 4]; out[i*2 + 1] = h[d[i] & 0xF]; }
    out[64] = '\0';
}

static void set_inprogress(bool v) {
    Preferences p;
    if (p.begin("ota", false)) {
        p.putBool("inprog", v);
        if (v) p.putBool("fs", s_is_fs);
        p.end();
    }
}

static void mark_boot_success() {
    Preferences p;
    if (p.begin("ota", false)) {
        p.putBool("ok", true);
        p.putBool("fs", s_is_fs);
        if (s_is_fs) p.putString("fsver", s_version);
        p.end();
    }
}

static void error_code(OtaResult res, char* out, size_t len) {
    switch (res) {
        case OtaResult::OK:          strlcpy(out, "ok",                 len); break;
        case OtaResult::PAUSED:      strlcpy(out, "paused",             len); break;
        case OtaResult::ERR_META:    strlcpy(out, "meta_failed",        len); break;
        case OtaResult::ERR_BEGIN:   strlcpy(out, "flash_begin_failed", len); break;
        case OtaResult::ERR_TIMEOUT: strlcpy(out, "chunk_timeout",      len); break;
        case OtaResult::ERR_WRITE:   strlcpy(out, "write_failed",       len); break;
        case OtaResult::ERR_SHA:     strlcpy(out, "sha_mismatch",       len); break;
        case OtaResult::ERR_END:     strlcpy(out, "verify_failed",      len); break;
        case OtaResult::ERR_FS_SIZE: strlcpy(out, "fs_too_big",         len); break;
        case OtaResult::ERR_TARGET:  strlcpy(out, "target_mismatch",    len); break;
        default:                     strlcpy(out, "unknown",            len); break;
    }
}

// ── Inbound hook — called from mqtt_broker.cpp's callback() for every message ──
bool TasksOTA::consume(const char* topic, const uint8_t* payload, unsigned int len) {
    if (!s_active) return false;

    if (strcmp(topic, s_topic_meta_res) == 0) {
        unsigned n = (len < sizeof(s_meta_json) - 1) ? len : sizeof(s_meta_json) - 1;
        memcpy(s_meta_json, payload, n);
        s_meta_json[n] = '\0';
        s_meta_ready = true;
        return true;
    }

    if (s_topic_chunk_res_pfx_len &&
        strncmp(topic, s_topic_chunk_res_pfx, s_topic_chunk_res_pfx_len) == 0) {
        s_chunk_off = strtoul(topic + s_topic_chunk_res_pfx_len, nullptr, 10);
        uint16_t n  = (len < sizeof(s_chunk_buf)) ? (uint16_t)len : (uint16_t)sizeof(s_chunk_buf);
        memcpy(s_chunk_buf, payload, n);
        s_chunk_len   = n;
        s_chunk_ready = true;
        return true;
    }
    return false;
}

// Do a meta/req -> meta/res handshake.
static bool fetch_meta(uint32_t& reqId, uint32_t* size, uint32_t* chunkSize,
                        char* sha, size_t shalen, char* ver, size_t verlen,
                        char* target, size_t tgtlen) {
    for (uint8_t a = 0; a < OTA_META_RETRIES; a++) {
        s_meta_ready = false;
        char req[48];
        snprintf(req, sizeof(req), "{\"reqId\":%lu}", (unsigned long)++reqId);
        bool pub_ok = client.publish(s_topic_meta_req, req);
        Serial.printf("OTA: meta/req attempt %u/%u %s (%s)\n",
                      a + 1, OTA_META_RETRIES, pub_ok ? "sent" : "PUBLISH FAILED", req);
        uint32_t t0 = millis();
        while ((uint32_t)(millis() - t0) < OTA_META_TIMEOUT_MS && !s_meta_ready) {
            client.loop();
            delay(2);
        }
        if (!s_meta_ready) { Serial.println(F("OTA: meta/res timeout")); continue; }
        Serial.printf("OTA: meta/res received: %s\n", s_meta_json);
        StaticJsonDocument<256> doc;
        if (deserializeJson(doc, s_meta_json)) continue;
        uint32_t    sz = doc["size"]      | 0UL;
        uint32_t    cs = doc["chunkSize"] | 1536U;
        const char* sh = doc["sha256"]    | "";
        if (sz > 0 && strlen(sh) == 64 && cs > 0 && cs <= sizeof(s_chunk_buf)) {
            *size = sz; *chunkSize = cs;
            strlcpy(sha, sh, shalen);
            strlcpy(ver, doc["version"] | "", verlen);
            strlcpy(target, doc["target"] | "", tgtlen);
            return true;
        }
    }
    return false;
}

// Pull chunks from s_offset up to s_size, writing + hashing each.
static OtaResult pull(uint32_t& reqId) {
    int last_pct = -1;
    while (s_offset < s_size) {
        uint16_t want = (uint16_t)((s_size - s_offset < s_chunkSize) ? (s_size - s_offset) : s_chunkSize);
        bool got = false;
        for (uint8_t a = 0; a < OTA_CHUNK_RETRIES && !got; a++) {
            s_chunk_ready = false;
            char req[80];
            snprintf(req, sizeof(req), "{\"reqId\":%lu,\"offset\":%lu,\"len\":%u}",
                     (unsigned long)++reqId, (unsigned long)s_offset, want);
            client.publish(s_topic_chunk_req, req);
            uint32_t t0 = millis();
            while ((uint32_t)(millis() - t0) < OTA_CHUNK_TIMEOUT_MS) {
                client.loop();
                if (s_chunk_ready) {
                    if (s_chunk_off == s_offset) { got = true; break; }
                    s_chunk_ready = false;
                }
                if (!client.connected()) break;
                delay(2);
            }
            if (!got && !client.connected()) break;
        }
        if (!got) {
            if (++s_pause_count > OTA_MAX_PAUSES) return OtaResult::ERR_TIMEOUT;
            s_paused = true;
            Serial.printf("OTA: stalled at %lu - pausing (%u/%u), resume on reconnect\n",
                          (unsigned long)s_offset, s_pause_count, OTA_MAX_PAUSES);
            return OtaResult::PAUSED;
        }
        if (Update.write(s_chunk_buf, s_chunk_len) != s_chunk_len) return OtaResult::ERR_WRITE;
        mbedtls_sha256_update(&s_sha, s_chunk_buf, s_chunk_len);
        s_offset += s_chunk_len;

        int pct = (int)((uint64_t)s_offset * 100 / s_size);
        if (pct != last_pct) {
            last_pct = pct;
            char st[72];
            snprintf(st, sizeof(st), "{\"state\":\"DOWNLOADING\",\"offset\":%lu,\"size\":%lu}",
                     (unsigned long)s_offset, (unsigned long)s_size);
            publish_ota_state(st);
            Serial.printf("OTA: %d%% (%lu/%lu)\n", pct, (unsigned long)s_offset, (unsigned long)s_size);
        }
    }
    return OtaResult::OK;
}

static OtaResult finalize() {
    uint8_t digest[32]; char sha_calc[68];
    mbedtls_sha256_finish(&s_sha, digest);
    mbedtls_sha256_free(&s_sha);
    hex32(digest, sha_calc);
    if (strcasecmp(sha_calc, s_sha_expect) != 0) {
        Serial.printf("OTA: sha mismatch calc=%s exp=%s\n", sha_calc, s_sha_expect);
        return OtaResult::ERR_SHA;
    }
    if (!Update.end()) {
        Serial.printf("OTA: end failed: %s\n", Update.errorString());
        return OtaResult::ERR_END;
    }
    return OtaResult::OK;
}

static void succeed() {
    char st[64];
    snprintf(st, sizeof(st), "{\"state\":\"VERIFIED\",\"version\":\"%s\"}", s_version);
    publish_ota_state(st);
    Serial.println(F("OTA: success - rebooting"));
    mark_boot_success();
    delay(400);
    ESP.restart();
}

static void fail(OtaResult r) {
    char code[24];
    error_code(r, code, sizeof(code));
    char st[72];
    snprintf(st, sizeof(st), "{\"state\":\"FAILED\",\"err\":\"%s\"}", code);
    publish_ota_state(st);
    Serial.printf("OTA: failed - %s\n", code);

    mbedtls_sha256_free(&s_sha);
    Update.abort();
    set_inprogress(false);
    s_topic_chunk_res_pfx_len = 0;
    s_paused = false;
    s_active = false;

    if (s_is_fs && s_fs_unmounted) {
        Serial.println(F("OTA: fs flash failed after unmount - rebooting to recover FS"));
        delay(400);
        ESP.restart();
    }
}

static OtaResult settle(OtaResult r) {
    if (r == OtaResult::PAUSED) return r;
    if (r != OtaResult::OK) { fail(r); return r; }
    r = finalize();
    if (r != OtaResult::OK) { fail(r); return r; }
    succeed();                 // reboots — not reached
    return OtaResult::OK;
}

// Start a fresh OTA. is_fs=false: app firmware -> "ota/" subtree, U_FLASH.
// is_fs=true: filesystem -> "otafs/" subtree, U_SPIFFS.
static OtaResult startSession(bool is_fs) {
    s_is_fs        = is_fs;
    s_fs_unmounted = false;
    build_topics(is_fs);
    s_active      = true;
    s_paused      = false;
    s_pause_count = 0;
    s_offset      = 0;

    publishStatus("{\"ota\":\"starting\"}");
    Serial.printf("OTA: session start (%s). meta/req -> %s, listening on %s\n",
                  is_fs ? "fs" : "fw", s_topic_meta_req, s_topic_meta_res);

    uint32_t reqId = millis();
    char target[12] = {0};
    if (!fetch_meta(reqId, &s_size, &s_chunkSize, s_sha_expect, sizeof(s_sha_expect),
                     s_version, sizeof(s_version), target, sizeof(target))) {
        Serial.println(F("OTA: no meta/res from server - giving up"));
        fail(OtaResult::ERR_META);
        return OtaResult::ERR_META;
    }
    Serial.printf("OTA(%s): size=%lu chunk=%lu ver=%s\n", is_fs ? "fs" : "fw",
                  (unsigned long)s_size, (unsigned long)s_chunkSize, s_version);

    if (is_fs) {
        // Wrong-image guard: the optional "target" echo in meta/res must say
        // "spiffs" (absent = accepted, for compatibility).
        if (target[0] != '\0' && strcmp(target, "spiffs") != 0) {
            Serial.printf("OTA(fs): meta target '%s' != spiffs - wrong image\n", target);
            fail(OtaResult::ERR_TARGET);
            return OtaResult::ERR_TARGET;
        }
        const esp_partition_t* part = esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
        uint32_t cap = part ? part->size : 0;
        if (s_size > cap) {
            Serial.printf("OTA(fs): image %lu > fs partition %lu\n",
                          (unsigned long)s_size, (unsigned long)cap);
            fail(OtaResult::ERR_FS_SIZE);
            return OtaResult::ERR_FS_SIZE;
        }
        // Unmount so no other task writes the partition underneath the flash
        // session; the only way out of a started fs flash is a reboot.
        SPIFFS.end();
        s_fs_unmounted = true;
    }

    if (!Update.begin(s_size, is_fs ? U_SPIFFS : U_FLASH)) {
        Serial.printf("OTA(%s): begin failed: %s\n", is_fs ? "fs" : "fw", Update.errorString());
        fail(OtaResult::ERR_BEGIN);
        return OtaResult::ERR_BEGIN;
    }
    set_inprogress(true);
    mbedtls_sha256_init(&s_sha);
    mbedtls_sha256_starts(&s_sha, 0);

    return settle(pull(reqId));
}

// Resume a paused OTA (stalled mid-download). Re-arms the server side by
// re-doing the meta handshake before continuing from the saved offset.
static OtaResult resumeSession() {
    if (!(s_active && s_paused)) return OtaResult::OK;
    s_paused = false;

    uint32_t reqId = millis();
    uint32_t size = 0, chunkSize = 0;
    char sha[72] = {0}, ver[24] = {0}, target[12] = {0};
    if (!fetch_meta(reqId, &size, &chunkSize, sha, sizeof(sha), ver, sizeof(ver),
                     target, sizeof(target))) {
        if (++s_pause_count > OTA_MAX_PAUSES) { fail(OtaResult::ERR_TIMEOUT); return OtaResult::ERR_TIMEOUT; }
        s_paused = true;
        return OtaResult::PAUSED;
    }
    if (size != s_size || strcasecmp(sha, s_sha_expect) != 0) {
        Serial.println(F("OTA: package changed mid-OTA - aborting"));
        fail(OtaResult::ERR_SHA);
        return OtaResult::ERR_SHA;
    }
    Serial.printf("OTA: resuming at %lu/%lu\n", (unsigned long)s_offset, (unsigned long)s_size);

    return settle(pull(reqId));
}

// ── Public API ─────────────────────────────────────────────────────────
// No task is created — OTA runs on the MQTT task (see service()). begin() just
// (re)stores the publish callback + status topic; safe to call on every
// reconnect.
bool TasksOTA::begin(OtaMqttPublishCb pubCb, const char* statusTopic,
                      uint32_t stackBytes, UBaseType_t priority, BaseType_t core) {
    (void)stackBytes; (void)priority; (void)core;
    sPub = pubCb;
    sStatusTopic = statusTopic;
    return true;
}

// Called from callback() (which runs inside client.loop() on the MQTT task).
// Only latches the request — the session itself must start AFTER the callback
// returns, so service() picks it up on the next mqtt_com_loop() iteration
// rather than re-entering client.loop() from within a callback.
bool TasksOTA::request(bool is_fs) {
    if (s_active) return false;      // an OTA is already running/paused
    if (s_run_pending) return false; // one already latched, not yet started
    s_run_is_fs   = is_fs;
    s_run_pending = true;
    return true;
}

// Run pending OTA work on the MQTT task. Call once per mqtt_com_loop()
// iteration, AFTER client.loop(). Starts a freshly requested session, or
// resumes a paused one once the link is back.
void TasksOTA::service() {
    if (s_run_pending && !s_active) {
        s_run_pending = false;
        Serial.printf("OTA: service - starting %s session\n", s_run_is_fs ? "fs" : "fw");
        startSession(s_run_is_fs);
    }
}

// Called from reconnectMQTT() (MQTT task context, before this iteration's
// client.loop()) — safe to resume inline; it's the only client user at that
// point.
bool TasksOTA::resumeIfPaused() {
    if (!(s_active && s_paused)) return false;
    Serial.println(F("OTA: reconnect - resuming paused session"));
    resumeSession();
    return true;
}

bool TasksOTA::pending() {
    return s_active && s_paused;
}

bool TasksOTA::active() {
    return s_active;
}

bool TasksOTA::inProgress() {
    return active();
}

void TasksOTA::bootCheck() {
    Preferences p;
    if (!p.begin("ota", false)) return;
    bool ok  = p.getBool("ok", false);
    bool inp = p.getBool("inprog", false);
    if (ok || inp) {
        s_boot_was_fs = p.getBool("fs", false);
        if (s_boot_was_fs) p.getString("fsver", s_boot_fsver, sizeof(s_boot_fsver));
    }
    if (ok) {
        p.putBool("ok", false);
        p.putBool("inprog", false);
        s_booted_after_ota = true;
        Serial.printf("OTA: booted after a successful %s flash\n", s_boot_was_fs ? "filesystem" : "firmware");
    } else if (inp) {
        p.putBool("inprog", false);
        s_booted_aborted = true;
        Serial.printf("OTA: booted after an interrupted %s OTA\n", s_boot_was_fs ? "filesystem" : "firmware");
    }
    p.end();
}

void TasksOTA::bootReportIfNeeded() {
    if (s_booted_after_ota) {
        s_booted_after_ota = false;
        char msg[64];
        if (s_boot_was_fs) {
            snprintf(msg, sizeof(msg), "{\"ota\":\"updated\",\"fs\":true,\"version\":\"%s\"}", s_boot_fsver);
        } else {
            snprintf(msg, sizeof(msg), "{\"ota\":\"updated\",\"fs\":false,\"version\":\"%s\"}", FW_VER);
        }
        publishStatus(msg);
    }
    if (s_booted_aborted) {
        s_booted_aborted = false;
        char msg[48];
        snprintf(msg, sizeof(msg), "{\"ota\":\"failed\",\"err\":\"aborted\",\"fs\":%s}", s_boot_was_fs ? "true" : "false");
        publishStatus(msg);
    }
}
