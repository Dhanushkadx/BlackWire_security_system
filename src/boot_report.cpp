#include "boot_report.h"
#include <Preferences.h>     // NVS — tiny cross-boot state
#include <esp_system.h>      // esp_reset_reason()
#include <esp_timer.h>       // esp_timer_get_time() — 64-bit monotonic uptime

static const char* NVS_KEY_ALIVE = "alive";   // uint32 UTC epoch

// ── Module state ─────────────────────────────────────────────────────────────
static BootReportConfig   s_cfg   = {};
static esp_reset_reason_t s_reason = ESP_RST_UNKNOWN;
static uint32_t           s_last_alive = 0;      // read at boot (≈ interrupt time)
static bool               s_event_sent = false;  // once-per-boot latch
static uint32_t           s_last_ckpt_ms = 0;    // rate-limit for checkpoints

void boot_report_init(const BootReportConfig& cfg) {
    s_cfg = cfg;
    if (!s_cfg.nvs_ns)        s_cfg.nvs_ns = "bootrep";
    if (s_cfg.checkpoint_ms == 0) s_cfg.checkpoint_ms = 30000;

    // esp_reset_reason() is stable for the whole session — exact call time is moot.
    s_reason = esp_reset_reason();

    Preferences p;
    if (p.begin(s_cfg.nvs_ns, true)) {           // read-only
        s_last_alive = p.getUInt(NVS_KEY_ALIVE, 0);
        p.end();
    }
    Serial.printf("boot_report: reset=%s last_alive=%lu\n",
                  boot_report_reason_str(), (unsigned long)s_last_alive);
}

void boot_report_checkpoint(uint32_t min_interval_ms) {
    uint32_t now_epoch = s_cfg.now ? s_cfg.now() : 0;
    if (now_epoch == 0) return;                  // clock not set → nothing meaningful to store

    uint32_t interval = min_interval_ms ? min_interval_ms : s_cfg.checkpoint_ms;

    // Rate-limit on the monotonic clock (never the wall clock — a time step must
    // not disturb the cadence).
    uint32_t nowms = millis();
    if (s_last_ckpt_ms != 0 && (nowms - s_last_ckpt_ms) < interval) return;
    s_last_ckpt_ms = nowms;

    Preferences p;
    if (p.begin(s_cfg.nvs_ns, false)) {          // read-write
        p.putUInt(NVS_KEY_ALIVE, now_epoch);
        p.end();
    }
}

void boot_report_tick_emit() {
    if (s_event_sent) return;
    if (!s_cfg.emit) return;

    // Need the clock for a real timestamp. On a WiFi/GSM unit with no RTC this
    // arrives seconds (NTP/NITZ) — or, if the link was down, much later — after
    // boot. We wait for it; with a durable log downstream, delivery is handled
    // separately, so we do NOT need the link here.
    uint32_t boot_epoch = boot_report_boot_epoch();
    if (boot_epoch == 0) return;                 // clock not ready yet — try again next tick
    s_event_sent = true;

    char detail[160];
    if (s_last_alive > 0 && boot_epoch >= s_last_alive) {
        // We had a last-alive checkpoint → bound the outage. outage excludes any
        // offline gap because boot_epoch is the true boot instant (uptime anchor),
        // not the (possibly much later) moment the link/clock came up.
        unsigned long outage = (unsigned long)boot_epoch - s_last_alive;
        snprintf(detail, sizeof(detail),
                 "{\"reset_reason\":\"%s\",\"interrupt_t\":%lu,\"restore_t\":%lu,\"outage_s\":%lu}",
                 boot_report_reason_str(), (unsigned long)s_last_alive,
                 (unsigned long)boot_epoch, outage);
    } else {
        // No checkpoint (idle before the reboot, or first boot) — report the reason.
        snprintf(detail, sizeof(detail),
                 "{\"reset_reason\":\"%s\",\"restore_t\":%lu}",
                 boot_report_reason_str(), (unsigned long)boot_epoch);
    }

    // Stamp the record at the boot instant (not the send time) — the boot happened
    // then, even if we could only report it now.
    s_cfg.emit("power", -1, "boot", detail, boot_epoch);
}

const char* boot_report_reason_str() {
    switch (s_reason) {
        case ESP_RST_POWERON:   return "poweron";    // cold power-up (or the mains dip that killed us)
        case ESP_RST_BROWNOUT:  return "brownout";   // supply sag tripped the ESP32 BOD
        case ESP_RST_PANIC:     return "panic";      // firmware crash / exception
        case ESP_RST_TASK_WDT:  return "task_wdt";   // a task hung (e.g. the valve-hang bug)
        case ESP_RST_INT_WDT:   return "int_wdt";    // interrupt watchdog
        case ESP_RST_SW:        return "sw";         // deliberate ESP.restart()
        case ESP_RST_DEEPSLEEP: return "deepsleep";
        default:                return "unknown";
    }
}

uint32_t boot_report_last_alive() { return s_last_alive; }

uint32_t boot_report_boot_epoch() {
    uint32_t now_epoch = s_cfg.now ? s_cfg.now() : 0;
    if (now_epoch == 0) return 0;                // clock not ready
    // boot_epoch = now − seconds-since-boot. 64-bit esp_timer → no millis() wrap
    // even after a very long uptime/outage.
    uint32_t uptime_s = (uint32_t)(esp_timer_get_time() / 1000000LL);
    return now_epoch - uptime_s;
}
