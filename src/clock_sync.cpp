#include "clock_sync.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <sys/time.h>
#include "statments.h"      // rtc (display clock)
#include "gsm_broker.h"     // timesync_need — asks the GSM task for a fresh CCLK
#include "boot_report.h"    // checkpoint rewrite after a clock step

// Plausibility window for ANY source. A SIM800 answers +CCLK before it has
// registered on the network, and what it returns then is garbage (typically an
// 80s/2000s date) — accepting it would set a wrong clock that then looks valid
// forever. Bounds: 2024-01-01 .. 2100-01-01.
#define EPOCH_MIN 1704067200UL
#define EPOCH_MAX 4102444800UL

// How long GSM gets before NTP is allowed to answer instead. GSM registration
// plus CCLK normally lands well under a minute; 3 min keeps NTP from pre-empting
// a merely slow network.
#define GSM_GRACE_MS        (3UL * 60UL * 1000UL)
#define NTP_RETRY_MS        (30UL * 1000UL)
#define RESYNC_INTERVAL_MS  (24UL * 60UL * 60UL * 1000UL)

// A step larger than this means the two sources disagreed about more than drift,
// so the last-alive checkpoint was written against a different base and must be
// rewritten before it is used for outage arithmetic.
#define CLOCK_STEP_GUARD_S  60

static bool     s_ready          = false;
static uint32_t s_last_sync_ms   = 0;
static uint32_t s_last_ntp_ms    = 0;
static uint32_t s_need_since_ms  = 1;   // 1 = "needed since boot"; 0 = satisfied
static bool     s_ntp_begun      = false;

static WiFiUDP   s_ntp_udp;
// Offset 0 — this client hands back UTC. The local offset lives on `rtc`, for
// display only.
static NTPClient s_ntp(s_ntp_udp, "pool.ntp.org", 0, 60000);

bool clock_ready(){ return s_ready; }

uint32_t clock_now_utc(){
    if (!s_ready) return 0;          // the sentinel boot_report depends on
    return (uint32_t)time(nullptr);
}

bool clock_apply_epoch(uint32_t utc_epoch, const char* src){
    if (utc_epoch < EPOCH_MIN || utc_epoch >= EPOCH_MAX) {
        Serial.printf_P(PSTR("clock: REJECT %s epoch=%lu (implausible)\n"),
                        src ? src : "?", (unsigned long)utc_epoch);
        return false;
    }

    uint32_t before = s_ready ? (uint32_t)time(nullptr) : 0;

    struct timeval tv;
    tv.tv_sec  = (time_t)utc_epoch;
    tv.tv_usec = 0;
    settimeofday(&tv, nullptr);

    s_ready         = true;
    s_need_since_ms = 0;
    s_last_sync_ms  = millis();
    if (s_last_sync_ms == 0) s_last_sync_ms = 1;   // 0 means "never" elsewhere

    // A big step invalidates the base the last-alive marker was written under.
    // Rewrite it now so any outage computed later is measured against one clock.
    if (before) {
        int32_t step = (int32_t)((int64_t)utc_epoch - (int64_t)before);
        if (step > CLOCK_STEP_GUARD_S || step < -CLOCK_STEP_GUARD_S) {
            Serial.printf_P(PSTR("clock: step %ld s — rewriting alive marker\n"), (long)step);
            boot_report_checkpoint(1);   // 1 ms interval = force
        }
    }

    Serial.printf_P(PSTR("clock: set from %s, utc=%lu\n"),
                    src ? src : "?", (unsigned long)utc_epoch);
    return true;
}

void clock_sync_tick(){
    uint32_t now = millis();

    // Daily re-sync. GSM is asked first (timesync_need is what its task polls);
    // NTP only steps in below if GSM has not answered within the grace period.
    if (s_need_since_ms == 0) {
        if (!s_ready) return;
        if ((now - s_last_sync_ms) < RESYNC_INTERVAL_MS) return;
        s_need_since_ms = now ? now : 1;
        timesync_need   = true;
        Serial.println(F("clock: daily re-sync due"));
        return;
    }

    // A sync is outstanding — give GSM its head start before falling back.
    if ((now - s_need_since_ms) < GSM_GRACE_MS) return;
    if (WiFi.status() != WL_CONNECTED)          return;
    if (s_last_ntp_ms && (now - s_last_ntp_ms) < NTP_RETRY_MS) return;
    s_last_ntp_ms = now ? now : 1;

    if (!s_ntp_begun) { s_ntp.begin(); s_ntp_begun = true; }
    if (s_ntp.forceUpdate()) {
        clock_apply_epoch((uint32_t)s_ntp.getEpochTime(), "ntp");
    } else {
        Serial.println(F("clock: NTP update failed"));
    }
}
