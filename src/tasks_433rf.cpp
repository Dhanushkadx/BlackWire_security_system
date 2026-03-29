#include "tasks_433rf.h"

// Your existing project headers (adjust include paths to match your project)
#include "statments.h"              // EventRTOS_buzzer, TASK_7_BIT, timers, etc.
#include "rf_methods.h"             // if you have remotes logic there (optional)
#include "providers/prov_rf_ev1527_readable.h"  // ProvRF_EV1527

// ------------------- External objects from your project -------------------
extern RCSwitch mySwitch;

// These exist in your current code base:
extern bool rf_id_automatic_clr_timer_en;
extern unsigned long prev_RFID;
extern TimerSW Timer_rf_id_auto_clr;

// Buzzer event group
extern EventGroupHandle_t EventRTOS_buzzer;

// Your RF provider instance (the one that maps RF codes to zone IDs)
extern ProvRF_EV1527 provRf;

// ------------------- Internal RF queue -------------------
static QueueHandle_t qRfCodes = nullptr;

// Tuning
static constexpr uint8_t  RF_QUEUE_LEN        = 32;
static constexpr uint16_t RF_POLL_PERIOD_MS   = 2;    // poll fast enough for RCSwitch
static constexpr uint16_t RF_DUP_BLOCK_MS     = 1000;  // filter repeated codes
static constexpr uint16_t RF_IDLE_CLEAR_MS    = 3000; // your old timer

// Repeat filter (in addition to RCSwitch duplicates)
static uint32_t lastCode_ = 0;
static uint32_t lastCodeMs_ = 0;

static inline void rf_queue_init()
{
  if (qRfCodes) return;

  qRfCodes = xQueueCreate(RF_QUEUE_LEN, sizeof(uint32_t));
  if (!qRfCodes) {
    Serial.println(F("RF433: qRfCodes create failed"));
  } else {
    Serial.println(F("RF433: qRfCodes initialized"));
  }
}

static inline void rf_queue_push(uint32_t code)
{
  if (!qRfCodes) return;
  // Non-blocking: drop if queue full (better than blocking alarm/system)
  xQueueSendToBack(qRfCodes, &code, 0);
}

// ------------------- Your polling function (moved here) -------------------
void RFListiner()
{
  if (mySwitch.available())
  {
    const uint32_t code = (uint32_t)mySwitch.getReceivedValue();
    mySwitch.resetAvailable();
    Serial.print(F("RF code received: "));
    Serial.println(code);
    wsTxLog("RF", "Received code 123456");

    // Ignore invalid/noise codes
    if (code == 0) return;

    const uint32_t now = millis();

    // Extra repeat filter (helps when receiver spams same frame)
    if (code == lastCode_ && (uint32_t)(now - lastCodeMs_) < RF_DUP_BLOCK_MS) {
      return;
    }
    lastCode_ = code;
    lastCodeMs_ = now;

    // Your old "prev_RFID" duplicate suppression
    if (code != (uint32_t)prev_RFID)
    {
      prev_RFID = code;

      rf_id_automatic_clr_timer_en = true;
      Timer_rf_id_auto_clr.interval = RF_IDLE_CLEAR_MS;
      Timer_rf_id_auto_clr.previousMillis = now;

      // Feedback beep on RF activity (optional)
      xEventGroupSetBits(EventRTOS_buzzer, TASK_7_BIT);

      // ✅ Just queue the code. No SPIFFS, no alarm actions here.
      rf_queue_push(code);
    }
  }

  // Auto-clear “prev_RFID” after idle (your existing behavior)
  if (rf_id_automatic_clr_timer_en && Timer_rf_id_auto_clr.Timer_run())
  {
    rf_id_automatic_clr_timer_en = false;
    prev_RFID = 0;
  }
}

static int8_t remcode_u32(uint32_t code, int8_t *remoteUserID)
{
  uint8_t split_4bit = code & 0x0F;
  uint8_t split_8bit = code & 0xFF;

  int8_t command = -1;
  uint32_t remoteID = 0;
//Serial.printf_P(PSTR("remcode_u32: code=%u split4=%02X split8=%02X\n"), code, split_4bit, split_8bit);
  if (split_4bit == 0x01 || split_4bit == 0x02 || split_4bit == 0x04 || split_4bit == 0x08) {
    command  = code & 0x0F;
    remoteID = code >> 4;
    *remoteUserID = comp_remote_RFID(remoteID, 4);
    
  }
  else if (split_8bit == 0xC0 || split_8bit == 0x03 || split_8bit == 0x0C || split_8bit == 0x30) {
    command  = code & 0xFF;
    remoteID = code >> 8;
    *remoteUserID = comp_remote_RFID(remoteID, 8);
  }

  switch (command) {
    case 0x01:
    case 0xC0: return 1; // Arm
    case 0x02:
    case 0x03: return 2; // Disarm
    case 0x04:
    case 0x0C: return 3; // Away
    case 0x08:
    case 0x30: return 4; // Panic
    default:   return -1;
  }
}

static bool rfRemoteHandleCode(uint32_t code)
{
  int8_t user = -1;
  int8_t cmd  = remcode_u32(code, &user);
  Serial.printf("[RF] code=%lu cmd=%d user=%d\n", (unsigned long)code, (int)cmd, (int)user);
  if (cmd == -1 || user == -1) return false;

  switch (cmd)
  {
    case 1: // ARM
      myAlarm_pannel.set_arm_mode(AS_ITIS_NO_BYPASS);
      myAlarm_pannel.set_system_state(SYS1_IDEAL, RF, user);
#ifdef MQTT_OK
      mqtt_publish_state_action_event("arm", (uint8_t)user, RF);
      mqtt_publish_latest_attributes();
      mqtt_publish_telemetry();
#endif
      break;

    case 2: // DISARM
      myAlarm_pannel.set_system_state(DEACTIVE, RF, user);
      eCurrent_state = DEACTIVE;
#ifdef MQTT_OK
      mqtt_publish_state_action_event("disarm", (uint8_t)user, RF);
      mqtt_publish_latest_attributes();
      mqtt_publish_telemetry();
#endif
      break;

    case 3: // AWAY
      // TODO
      break;

    case 4: // PANIC
      myAlarm_pannel.set_system_state(ALARM_CALLING, RF, user);
#ifdef MQTT_OK
      mqtt_publish_alarm_event("alarm_triggered", -1, "panic", "panic");
      mqtt_publish_latest_attributes();
      mqtt_publish_telemetry();
#endif
      break;
  }

  return true;
}


// ------------------- RF Scan session (portal learn mode) -------------------
static volatile ScanTarget g_scanTarget = SCAN_NONE;
static volatile int8_t    g_pendingSlot = -1;         // remote slot 1..8 (or -1)
static volatile uint32_t  g_scanDeadlineMs = 0;

void rfScanArm(ScanTarget target, int8_t slot, uint32_t timeoutMs)
{
  g_scanTarget = target;
  g_pendingSlot = slot;
  g_scanDeadlineMs = millis() + timeoutMs;

  Serial.printf("RF scan armed target=%u slot=%d timeout=%ums\n",
                (unsigned)target, (int)slot, (unsigned)timeoutMs);
}

void rfScanCancel()
{
  g_scanTarget = SCAN_NONE;
  g_pendingSlot = -1;
  g_scanDeadlineMs = 0;
}

// ------------------- Tasks -------------------
static void rf_poll_task(void*)
{
  for (;;)
  {
    RFListiner();
    vTaskDelay(pdMS_TO_TICKS(RF_POLL_PERIOD_MS));
  }
}

static void rf_process_task(void*)
{
  uint32_t code = 0;
    for (;;)
    {
        if (!qRfCodes) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        
        // If a scan is active, don't block forever. Wake up periodically to enforce timeout.
        TickType_t waitTicks = portMAX_DELAY;
        if (g_scanTarget != SCAN_NONE) {
            waitTicks = pdMS_TO_TICKS(100);
        }

        if (xQueueReceive(qRfCodes, &code, waitTicks) != pdPASS) {

            // No code received in this interval -> check timeout if scan active
            if (g_scanTarget != SCAN_NONE && (int32_t)(millis() - g_scanDeadlineMs) > 0) {
                const char* p = (g_scanTarget == SCAN_REMOTES) ? "remotes" : "zones";
                wsTxSendErr(p, "RF scan timeout");
                rfScanCancel();
            }
            continue;
        }

        // ------------------------------------------------------
        // RF Scan capture (portal learn mode)
        // Capture the NEXT RF code, report to portal, then stop scan.
        // ------------------------------------------------------
        if (g_scanTarget != SCAN_NONE) {
          Serial.println(F("RF code received during active scan session"));
            if ((int32_t)(millis() - g_scanDeadlineMs) > 0) {
                const char* p = (g_scanTarget == SCAN_REMOTES) ? "remotes" : "zones";
                wsTxSendErr(p, "RF scan timeout");
                Serial.println(F("RF scan session expired (timeout)"));
                rfScanCancel();
            } else {
                char codeStr[16];
                ultoa(code, codeStr, 10);

                if (g_scanTarget == SCAN_REMOTES) {
                    uint8_t slot = (g_pendingSlot > 0) ? (uint8_t)g_pendingSlot : 0;
                    wsTxSendScan("remotes", codeStr, slot);
                    Serial.println(F("RF scan session: captured remote code"));
                } else if (g_scanTarget == SCAN_ZONES) {
                  Serial.println(F("RF scan session: captured zone code"));
                    wsTxSendScan("zones", codeStr, 0);
                }

                rfScanCancel(); // one-shot capture
            }

            // NOTE: we still continue normal routing below (do NOT consume the code)
        }

        // 1) Try remote first (fast, RAM lookup)
            if (rfRemoteHandleCode(code)) {
                continue; // handled as remote → do NOT create zone events
            }

            // 2) Not remote → RF zones path
            provRf.onCode(code);   // rfMap → rawPush (momentary zones etc.)

            // Optional: if you still need legacy device-RFID mapping:
            // rfLegacyDeviceHandle(code);  // converts to zoneId and rawPush(...)
        }
 
}



// ------------------- Public init/start -------------------
void rf433_init()
{
  rf_queue_init();

  // Ensure RCSwitch receiver is enabled somewhere in your setup already.
  // If you prefer module ownership, you can do it here if you expose the pin:
   mySwitch.enableReceive(digitalPinToInterrupt(PIN_RF433MH));
}

void rf433_start_tasks()
{
  // Polling task: reads RCSwitch and pushes codes
  xTaskCreatePinnedToCore(rf_poll_task, "rf433_poll", 3072, nullptr, 2, nullptr, 1);

  // Processing task: zones + remotes
  xTaskCreatePinnedToCore(rf_process_task, "rf433_proc", 4096, nullptr, 3, nullptr, 1);
}
