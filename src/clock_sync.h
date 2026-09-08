#ifndef _CLOCK_SYNC_H
#define _CLOCK_SYNC_H

#include <Arduino.h>

// ── clock_sync — one timebase, one writer ────────────────────────────────────
// The ESP32 internal clock ALWAYS holds true UTC. Every source that learns the
// time (GSM AT+CCLK, NTP) funnels through clock_apply_epoch(), so the convention
// is established in exactly one place instead of being re-derived at each call
// site — which is how the old code ended up storing local time as if it were
// UTC (the +CCLK timezone field was parsed and then discarded, leaving the clock
// 19800 s ahead of real epoch on a +05:30 network).
//
// Local time for HUMANS is a display concern: `rtc` carries the local offset, so
// rtc.getTime()/getTimeStruct() render local exactly as before. Anything that
// needs a machine timestamp (boot_report, MQTT telemetry) calls clock_now_utc().
//
// Source policy: GSM is primary; NTP is the fallback once GSM has had its grace
// period AND WiFi is up. First valid source wins, then a daily re-sync corrects
// drift — safe now that both sources agree on UTC, so a re-sync is a sub-second
// nudge rather than a timezone-sized jump.

// Local timezone applied to the DISPLAY clock only (never to what we store or
// publish). +05:30. The GSM network's own offset overrides this once known.
#define LOCAL_TZ_OFFSET_S 19800

// How often the "I am still alive at T" marker is rewritten to NVS. The device
// is battery backed, so a mains cut does not kill it (that is reported through
// AC_DETECT instead) -- a real outage means the battery ran flat too, which is an
// hours-long event. 30 min resolution is ample and costs ~48 NVS writes/day.
#define BOOT_CHECKPOINT_MS (30UL * 60UL * 1000UL)

// Apply a UTC epoch from `src` ("gsm" | "ntp"). Rejects implausible values, so a
// modem that answers +CCLK before it has registered cannot poison the clock.
// Returns true if the clock was actually set.
bool clock_apply_epoch(uint32_t utc_epoch, const char* src);

// True once a source has set the clock. This is the flag boot_report's 0
// sentinel is built on — validate at the source, then trust the flag downstream.
bool clock_ready();

// Current UTC epoch, or 0 if the clock has never been set.
uint32_t clock_now_utc();

// Drives the NTP fallback and the daily re-sync. Call from a periodic task.
void clock_sync_tick();

#endif // _CLOCK_SYNC_H
