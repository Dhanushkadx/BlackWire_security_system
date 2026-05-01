#include "tasks_alarm.h"

// --- External types (from your project headers) ---
#include "typex.h"          // DataBuffer, enums (eInvoking_source, etc.)
#include "alarm.h"          // ALARM
#include "universalEventx.h"
#include "event_bus.h"      // zonePop(), ZoneEvent
#include "TimerSW.h"


extern bool stringComplete_at_serial0;
extern char inputString[];
extern size_t inputStringLen;
extern eInvoking_source last_invorker;

// Your alarm panel instance
extern ALARM myAlarm_pannel;

// Timer(s) used elsewhere; not required here but kept if you later gate prints, etc.
extern TimerSW Timer_websocket_update;

// ------------------- Extern functions from your project -------------------
extern void RFListiner();

#ifdef GSM_OK
extern void ultimate_sms_hadlr();
extern void gsm_manager();
#endif

// Task handles (defined in main)
extern TaskHandle_t Task1;
extern TaskHandle_t Task2_sms;
extern TaskHandle_t Task4;

void Task1code(void *parameter)
{
  (void)parameter;

  TickType_t lastWake = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(10);

#ifdef MQTT_OK
  DataBuffer mqttMsg;
#endif

  for (;;)
  {
    vTaskDelayUntil(&lastWake, period);

    // ------------------------------------------------------------
    // 1) Consume inbound commands from MQTT (web/app)  [RX queue]
    // ------------------------------------------------------------
#ifdef MQTT_OK
    while (xQueueReceive(xQueue_mqtt_Qhdlr, &mqttMsg, 0) == pdPASS)
    {
      Serial.print(F("mqtt RX Queue data: "));
      Serial.println(mqttMsg.char_buffer_rx);

      universal_event_hadler(mqttMsg.char_buffer_rx, WEB, 0);
    }
#endif

    // ------------------------------------------------------------
    // 2) Consume zone events (changes) -> alarm + broadcast (fast)
    // ------------------------------------------------------------
    {
      ZoneEvent ze;
      const uint8_t MAX_EVENTS_PER_TICK = 20;
      uint8_t processed = 0;

      while (processed < MAX_EVENTS_PER_TICK && zonePop(ze, 0))
      {
        const uint8_t z = ze.zone;
        const uint8_t st = (uint8_t)ze.state;

        Serial.printf("Zone %u state %u\n", z, st);

        // ------------------------------------------------------------------
        // Handle fault zones separately from normal open/close.
        //
        // ZS_FAULT means the sensor wiring is broken (short/open circuit)
        // or the zone is chattering (flapping). We:
        //   1. Always set the trouble flag for MQTT reporting.
        //   2. Only trigger the alarm if the system is currently armed —
        //      a fault on a disarmed system is reported but not alarmed.
        // ------------------------------------------------------------------
        if (ze.state == ZS_FAULT) {
          Serial.printf("[ALARM] Zone %u FAULT — setting trouble\n", z);
#ifdef MQTT_OK
          mqtt_publish_telemetry(); // sends trouble=true via compute_trouble()
#endif
          // Trigger alarm only if system is armed
          const eMain_state curState = myAlarm_pannel.get_system_state();
          if (curState != DEACTIVE) {
            Serial.printf("[ALARM] Zone %u fault while armed — triggering alarm\n", z);
            myAlarm_pannel.Universal_zone_state_update(z);
          }
        } else {
          // 2.1 Normal open/close — update alarm logic
          myAlarm_pannel.Universal_zone_state_update(z);
        }

        // 2.2 Broadcast to comms router regardless of fault/normal
        // (portal and MQTT both need to display the fault state)
        ZoneBroadcast bz;
        bz.zone  = z;
        bz.state = st;
        bz.ts_ms = millis();
        broadcastPush(bz);

        processed++;
      }
    }

    // ------------------------------------------------------------
    // 3) Periodic alarm state machine / timers (must not starve)
    // ------------------------------------------------------------
    myAlarm_pannel.watcher();

    // ------------------------------------------------------------
    // 4) Serial CLI commands
    // ------------------------------------------------------------
    if (stringComplete_at_serial0)
    {
      stringComplete_at_serial0 = false;
      universal_event_hadler(inputString, last_invorker, 0);
      inputString[0] = '\0';
      inputStringLen = 0;
    }

    // ------------------------------------------------------------
    // 5) RF listener (kept as-is)
    // ------------------------------------------------------------
    //RFListiner();
  }
}

// ------------------- Task2: SMS handler -------------------
void Task2code_sms(void *parameter)
{
  (void)parameter;

  TickType_t lastWake = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(1);

  for (;;)
  {
    vTaskDelayUntil(&lastWake, period);
#ifdef GSM_OK
    ultimate_sms_hadlr();
#endif
  }
}

// ------------------- Task4: GSM manager -------------------
void Task4code_gsm_ctrl(void *parameter)
{
  (void)parameter;

  TickType_t lastWake = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(3000);

  for (;;)
  {
    vTaskDelayUntil(&lastWake, period);
#ifdef GSM_OK
    gsm_manager();
#endif
  }
}

// Convenience: create the tasks with your existing stack/prio/core choices.
void startAlarmTasks(){
  xTaskCreatePinnedToCore(Task1code,      "Task1",  4096,  nullptr, 1, &Task1,     0);
  xTaskCreatePinnedToCore(Task4code_gsm_ctrl,"Task4",4096,  nullptr, 4, &Task4,     1);
  xTaskCreatePinnedToCore(Task2code_sms,  "Task2", 6144,  nullptr, 2, &Task2_sms, 1);

}
