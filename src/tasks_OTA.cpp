// tasks_OTA.cpp
#include "tasks_OTA.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <SPIFFS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#define SAFE_CSTR(p) ((p) ? (p) : "(null)")

static const char* OTA_REQ_PATH    = "/ota_req.bin";
static const char* OTA_RESULT_PATH = "/ota_result.txt";
static const uint32_t OTA_MAGIC    = 0x4F544131; // 'OTA1'
static const size_t OTA_URL_MAX    = 256;

// ---------------- Persistent struct ----------------
#pragma pack(push, 1)
struct OtaReqFile {
  uint32_t magic;
  uint8_t  pending;   // 1=pending
  uint8_t  type;      // OtaType
  uint16_t rsv;
  char     url[OTA_URL_MAX]; // null terminated
  uint32_t crc;       // simple checksum
};
#pragma pack(pop)

static uint32_t crc32_simple(const uint8_t* p, size_t n) {
  // lightweight “good enough” checksum for corruption detection
  uint32_t c = 0xA5A5A5A5;
  for (size_t i = 0; i < n; i++) c = (c << 5) ^ (c >> 27) ^ p[i];
  return c;
}

// ---------------- Module state ----------------
static QueueHandle_t sQ = nullptr;
static TaskHandle_t  sTask = nullptr;
static volatile bool sInProgress = false;

static bool sBootLoaded = false;
static bool sOtaBootMode = false;

static OtaReqFile sPending = {}; // loaded at boot (if any)

static OtaMqttPublishCb sPub = nullptr;
static const char* sStatusTopic = nullptr;

// ---------------- File helpers ----------------
static bool writeReqFile(const OtaReqFile& r) {
  File f = SPIFFS.open(OTA_REQ_PATH, FILE_WRITE);
  if (!f) return false;
  size_t w = f.write((const uint8_t*)&r, sizeof(r));
  f.close();
  return w == sizeof(r);
}

static bool readReqFile(OtaReqFile& out) {
  File f = SPIFFS.open(OTA_REQ_PATH, FILE_READ);
  if (!f) return false;
  if (f.size() != (int)sizeof(OtaReqFile)) { f.close(); return false; }
  size_t r = f.read((uint8_t*)&out, sizeof(out));
  f.close();
  if (r != sizeof(out)) return false;

  if (out.magic != OTA_MAGIC) return false;

  uint32_t crcCalc = crc32_simple((const uint8_t*)&out, sizeof(out) - sizeof(out.crc));
  if (crcCalc != out.crc) return false;

  // Ensure null termination
  out.url[OTA_URL_MAX - 1] = '\0';
  return true;
}

static void writeResult(const char* line) {
  File f = SPIFFS.open(OTA_RESULT_PATH, FILE_WRITE);
  if (!f) return;
  f.println(line ? line : "");
  f.close();
}

static void publishStatus(const String& msg, bool retain=false) {
  if (sPub && sStatusTopic) {
    sPub(sStatusTopic, msg.c_str(), retain);
  }
}

// ---------------- HTTP redirect helper (safe) ----------------
static bool httpGetFollow(HTTPClient& http, WiFiClientSecure& client, int& httpCode) {
  // Follow up to 5 redirects manually (works reliably for GitHub)
  for (int i = 0; i < 5; i++) {
    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) return true;

    if (httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND ||
        httpCode == HTTP_CODE_TEMPORARY_REDIRECT || httpCode == HTTP_CODE_PERMANENT_REDIRECT) {

      String loc = http.getLocation();
      Serial.print("[OTA] Redirect -> ");
      Serial.println(loc.length() ? loc.c_str() : "(empty)");
      if (loc.length() == 0) return false;

      http.end();
      if (!http.begin(client, loc)) return false;
      continue;
    }

    return false;
  }
  return false;
}

// ---------------- Flashing ----------------
static bool startUpdateFromStream(WiFiClient* stream, int contentLength, int updateCommand) {
  if (!stream) {
    Serial.println("[OTA] stream null");
    return false;
  }

  if (!Update.begin(contentLength, updateCommand)) {
    Serial.printf("[OTA] Update.begin failed: %s\n", Update.errorString());
    return false;
  }

  size_t written = 0;
  int lastProgress = -1;

  const unsigned long timeoutMs = 120UL * 1000UL;
  unsigned long lastData = millis();

  while ((int)written < contentLength) {
    if (stream->available()) {
      uint8_t buf[512];
      size_t len = stream->read(buf, sizeof(buf));
      if (len > 0) {
        size_t w = Update.write(buf, len);
        if (w == 0) {
          Serial.printf("[OTA] Update.write failed: %s\n", Update.errorString());
          Update.abort();
          return false;
        }
        written += w;
        lastData = millis();

        int p = (int)((written * 100UL) / (unsigned long)contentLength);
        if (p != lastProgress) {
          Serial.printf("[OTA] Writing: %d%%\n", p);
          lastProgress = p;
        }
      }
    }

    if (millis() - lastData > timeoutMs) {
      Serial.println("[OTA] Timeout stalled. Abort.");
      Update.abort();
      return false;
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }

  if (!Update.end()) {
    Serial.printf("[OTA] Update.end failed: %s\n", Update.errorString());
    return false;
  }

  if (!Update.isFinished()) {
    Serial.println("[OTA] Update not finished!");
    return false;
  }

  return true;
}

static bool downloadAndApplySecure(const char* url, OtaType type) {
  if (!url || url[0] == '\0') {
    Serial.println("[OTA] ERROR: URL null/empty");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[OTA] WiFi not connected");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure(); // IMPORTANT: reduces TLS memory usage

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  Serial.print("[OTA] GET ");
  Serial.println(url);

  if (!http.begin(client, url)) {
    Serial.println("[OTA] http.begin failed");
    http.end();
    return false;
  }

  int httpCode = -1;
  bool okGet = httpGetFollow(http, client, httpCode);

  Serial.printf("[OTA] HTTP code: %d\n", httpCode);

  if (!okGet || httpCode != HTTP_CODE_OK) {
    Serial.printf("[OTA] Download failed. code=%d\n", httpCode);
    http.end();
    return false;
  }

  int contentLength = http.getSize();
  Serial.printf("[OTA] Content-Length: %d\n", contentLength);
  if (contentLength <= 0) {
    Serial.println("[OTA] Invalid content length");
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  int cmd = (type == OtaType::Spiffs) ? U_SPIFFS : U_FLASH;

  publishStatus(String("{\"ota\":\"downloading\",\"len\":") + contentLength + "}", false);

  bool ok = startUpdateFromStream(stream, contentLength, cmd);
  http.end();

  if (ok) {
    writeResult("OK");
    publishStatus("{\"ota\":\"success\",\"action\":\"reboot\"}", true);
    delay(300);
    ESP.restart();
  } else {
    writeResult("FAIL");
    publishStatus("{\"ota\":\"failed\"}", true);
  }
  return ok;
}

// ---------------- OTA task ----------------
struct OtaReqRam {
  char url[OTA_URL_MAX];
  OtaType type;
};

static void otaTask(void*) {
  OtaReqRam req{};
  for (;;) {
    if (xQueueReceive(sQ, &req, portMAX_DELAY) == pdTRUE) {
      sInProgress = true;

      publishStatus("{\"ota\":\"starting\"}", false);

      bool ok = downloadAndApplySecure(req.url, req.type);

      // If success: reboot already happened.
      // If fail: continue normal mode (your main can decide what to do).
      sInProgress = false;
      (void)ok;
    }
  }
}

// ---------------- Public API ----------------
namespace TasksOTA {

void bootLoadPending() {
  if (sBootLoaded) return;
  sBootLoaded = true;

  OtaReqFile r{};
  if (!readReqFile(r)) {
    sOtaBootMode = false;
    return;
  }

  if (r.pending == 1 && r.url[0] != '\0') {
    sPending = r;
    sOtaBootMode = true;
  } else {
    sOtaBootMode = false;
  }
}

bool isOtaBootMode() {
  return sOtaBootMode;
}

bool markPendingAndReboot(const char* url, OtaType type, OtaMqttPublishCb pubCb, const char* statusTopic) {
  if (!url || url[0] == '\0') return false;

  // Save publisher for immediate “restarting” status (optional)
  sPub = pubCb;
  sStatusTopic = statusTopic;

  OtaReqFile r{};
  memset(&r, 0, sizeof(r));
  r.magic = OTA_MAGIC;
  r.pending = 1;
  r.type = (uint8_t)type;
  strlcpy(r.url, url, sizeof(r.url));
  r.crc = crc32_simple((const uint8_t*)&r, sizeof(r) - sizeof(r.crc));

  if (!writeReqFile(r)) return false;

  if (sPub && sStatusTopic) {
    sPub(sStatusTopic, "{\"ota\":\"pending\",\"action\":\"reboot\"}", true);
  }

  delay(200);
  ESP.restart();
  return true; // not reached
}

bool begin(OtaMqttPublishCb pubCb, const char* statusTopic,
           uint32_t stackBytes, UBaseType_t priority, BaseType_t core) {

  sPub = pubCb;
  sStatusTopic = statusTopic;

  if (!sQ) {
    sQ = xQueueCreate(2, sizeof(OtaReqRam));
    if (!sQ) return false;
  }

  if (!sTask) {
    BaseType_t ok = xTaskCreatePinnedToCore(
      otaTask, "otaTask",
      stackBytes / sizeof(StackType_t),
      nullptr,
      priority,
      &sTask,
      core
    );
    if (ok != pdPASS) {
      sTask = nullptr;
      return false;
    }
  }

  return true;
}

bool startFromPending() {
  if (!sOtaBootMode) return false;
  if (!sQ) return false;

  // Clear pending flag FIRST to avoid reboot loop
  OtaReqFile cleared = sPending;
  cleared.pending = 0;
  cleared.crc = crc32_simple((const uint8_t*)&cleared, sizeof(cleared) - sizeof(cleared.crc));
  writeReqFile(cleared);

  // Enqueue OTA
  OtaReqRam r{};
  memset(&r, 0, sizeof(r));
  strlcpy(r.url, sPending.url, sizeof(r.url));
  r.type = (OtaType)sPending.type;

  publishStatus("{\"ota\":\"boot_mode\",\"step\":\"queue\"}", false);
  return xQueueSend(sQ, &r, 0) == pdTRUE;
}

bool inProgress() {
  return sInProgress;
}

} // namespace TasksOTA