#ifndef BOOT_REPORT_H
#define BOOT_REPORT_H

#include <Arduino.h>

// ── boot_report — reset-reason + boot/outage reporting (portable ESP32 module) ─
// Answers "why and when did this device reboot, and how long was it down?" for
// any ESP32 project. It is transport- and clock-agnostic: you inject a now()
// (epoch or 0) and an emit sink; it uses NVS for the tiny cross-boot state.
//
// Three pieces:
//   1. Reset reason — latched from esp_reset_reason() at boot.
//   2. Last-alive checkpoint — a periodic "I'm still running at epoch T" written
//      to NVS. The APP decides WHEN to call it (e.g. only while busy); this module
//      just rate-limits and persists. On the next boot it reads back ≈ the moment
//      power/execution was lost.
//   3. Uptime anchor — boot_epoch = now − uptime, giving the ACTUAL boot instant
//      once the clock is set, no matter how long the link took to come up.
//
// The once-per-boot "power/boot" event (reason + restore time + outage, if a
// checkpoint existed) is emitted via the injected sink as soon as the clock is
// ready. With a durable log downstream it does not need the link — recording is
// decoupled from sending.

// Current UTC epoch, or 0 if the clock is not set yet.
typedef uint32_t (*boot_now_fn)();
// Deliver one event. Signature matches the app's event emitter so it can be
// passed directly. body = JSON detail object (e.g. "{\"reset_reason\":\"sw\"}").
typedef void (*boot_emit_fn)(const char* type, int id, const char* outcome,
                             const char* detail_json, uint32_t ts);

struct BootReportConfig {
    boot_now_fn  now;                 // clock source (0 = not ready)
    boot_emit_fn emit;                // event sink (e.g. event_log_emit)
    const char*  nvs_ns;              // NVS namespace, e.g. "bootrep"
    uint32_t     checkpoint_ms;       // min interval between checkpoints (e.g. 30000)
};

// Call once early in setup(): latch reset reason + read the last-alive epoch.
void boot_report_init(const BootReportConfig& cfg);

// Persist "alive at now()". Call as often as you like from wherever makes sense;
// internally rate-limited to `min_interval_ms` and a no-op until the clock is set.
// Cheap NVS write of one uint32.
//
// The interval is a PARAMETER, not fixed config, because the right cadence depends
// on what the app is doing: a device mid-task wants a tight marker (the marker's
// lag is the error bar on any "how much did we lose" arithmetic), while an idle
// device only needs enough resolution to bound an outage. Passing 0 uses the
// configured default.
//
// IMPORTANT: whatever calls this defines what "last alive" MEANS. If it is only
// called while busy, the marker freezes when idle and any outage computed from it
// will span periods the device was demonstrably up.
void boot_report_checkpoint(uint32_t min_interval_ms = 0);

// Call periodically (e.g. every few seconds): emits the once-per-boot power/boot
// event the instant the clock is ready. Self-latches — fires exactly once.
void boot_report_tick_emit();

// Accessors.
const char* boot_report_reason_str();  // "poweron","brownout","panic","task_wdt",…
uint32_t    boot_report_last_alive();  // last-alive epoch from the previous run (0 if none)
uint32_t    boot_report_boot_epoch();  // uptime-anchored boot instant (0 if clock not ready)

#endif // BOOT_REPORT_H
