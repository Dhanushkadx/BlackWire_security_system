#include "ws_tx_queue.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

static AsyncWebSocket* g_ws = nullptr;

static QueueHandle_t g_qWsTx = nullptr;
static TaskHandle_t  g_wsTxTask = nullptr;

static inline void safeCopy(char* dst, size_t dstSize, const char* src) {
  if (!dst || dstSize == 0) return;
  if (!src) { dst[0] = '\0'; return; }
  strlcpy(dst, src, dstSize);
}

static void ws_tx_task(void* param) {
  (void)param;

  WsMsg m{};
  for (;;) {
    if (xQueueReceive(g_qWsTx, &m, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    // If WS not attached yet, drop message quietly.
    if (!g_ws) continue;

    // Build JSON here (single place in the system)
    StaticJsonDocument<256> doc;

    doc["ts"] = m.ts;

    switch (m.type) {
      case WS_EVT_OK:
        doc["respHeader"] = "ok";
        doc["page"] = m.page;
        doc["message"] = m.message;
        break;

      case WS_EVT_ERR:
        doc["respHeader"] = "err";
        doc["page"] = m.page;
        doc["message"] = m.message;
        break;

      case WS_EVT_SCAN_CODE:
        doc["respHeader"] = "data";
        doc["page"] = m.page;
        doc["scan_rfid"] = m.message;  // message holds code string
        if (m.v0 != 0) doc["slot"] = m.v0; // slot or index optional
        break;

     case WS_EVT_LOG:
        doc["respHeader"] = "log";
        doc["src"] = m.page;
        doc["msg"] = m.message;
        break;

      case WS_EVT_DATAX:
      default:
        doc["respHeader"] = "data";
        doc["page"] = m.page;
        if (m.message[0]) doc["message"] = m.message;
        if (m.v0 != 0) doc["v0"] = m.v0;
        if (m.v1 != 0) doc["v1"] = m.v1;
        break;
    }

    char out[256];
    size_t n = serializeJson(doc, out, sizeof(out));
    if (n > 0) {
      // One client only → broadcast is fine and simple.
      Serial.printf("WS TX: %s\n", out);
      g_ws->textAll(out, n);
    }
  }
}

void wsTxAttach(AsyncWebSocket* ws) {
  g_ws = ws;
}

bool wsTxBegin(uint16_t queueDepth, uint16_t taskStackWords, UBaseType_t taskPrio) {
  if (!g_qWsTx) {
    g_qWsTx = xQueueCreate(queueDepth, sizeof(WsMsg));
    if (!g_qWsTx) return false;
  }

  if (!g_wsTxTask) {
    BaseType_t ok = xTaskCreate(
      ws_tx_task,
      "ws_tx",
      taskStackWords,   // stack size in WORDS for ESP32 Arduino FreeRTOS
      nullptr,
      taskPrio,
      &g_wsTxTask
    );
    if (ok != pdPASS) {
      g_wsTxTask = nullptr;
      return false;
    }
  }

  return true;
}

void wsTxClear() {
  if (!g_qWsTx) return;
  xQueueReset(g_qWsTx);
}

static bool pushMsg(const WsMsg& msg) {
  if (!g_qWsTx) return false;
  return xQueueSend(g_qWsTx, &msg, 0) == pdTRUE;
}

bool wsTxSendOk(const char* page, const char* msg) {
  WsMsg m{};
  m.type = WS_EVT_OK;
  safeCopy(m.page, sizeof(m.page), page);
  safeCopy(m.message, sizeof(m.message), msg);
  m.ts = millis();
  return pushMsg(m);
}

bool wsTxSendErr(const char* page, const char* msg) {
  WsMsg m{};
  m.type = WS_EVT_ERR;
  safeCopy(m.page, sizeof(m.page), page);
  safeCopy(m.message, sizeof(m.message), msg);
  m.ts = millis();
  return pushMsg(m);
}

bool wsTxSendData(const char* page, const char* msg, uint32_t v0, uint32_t v1) {
  WsMsg m{};
  m.type = WS_EVT_DATAX;
  safeCopy(m.page, sizeof(m.page), page);
  safeCopy(m.message, sizeof(m.message), msg);
  m.v0 = v0;
  m.v1 = v1;
  m.ts = millis();
  return pushMsg(m);
}

bool wsTxSendScan(const char* page, const char* codeStr, uint32_t slotOrIndex) {
  WsMsg m{};
  m.type = WS_EVT_SCAN_CODE;
  safeCopy(m.page, sizeof(m.page), page);
  safeCopy(m.message, sizeof(m.message), codeStr);
  m.v0 = slotOrIndex;
  m.ts = millis();
  return pushMsg(m);
}

bool wsTxLog(const char* src, const char* msg)
{
  WsMsg m{};
  m.type = WS_EVT_LOG;
  safeCopy(m.page, sizeof(m.page), src);     // reuse page as source tag
  safeCopy(m.message, sizeof(m.message), msg);
  m.ts = millis();

  return pushMsg(m);
}