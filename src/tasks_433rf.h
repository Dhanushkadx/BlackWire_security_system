#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <RCSwitch.h>
#include "alarm.h"
#include "mqtt_brokerx.h"   // for publish_system_state
#include "statments.h"     // RF enum, eCurrent_state, etc.
#include "ws_tx_queue.h"    // for wsTxSendData (optional, for WS updates)

// Init queue + start tasks
void rf433_init();
void rf433_start_tasks();

// Optional: if you want to call polling manually (not recommended if using rf433_start_tasks)
void RFListiner();

// If you want to hook remote handling without coupling modules:
// Return true if handled as remote (ARM/DISARM/PANIC/etc)
//bool rfRemoteHandleCode(uint32_t code);

// Queue handle is internal (not exposed)

// -----------------------------------------------------------------------------
// RF Scan Session API (used by portal 'Scan' buttons)
// -----------------------------------------------------------------------------
enum ScanTarget : uint8_t {
  SCAN_NONE = 0,
  SCAN_ZONES,
  SCAN_REMOTES
};

// Arm a scan window. The NEXT RF code received will be reported over WS and scan ends.
// slot: for remotes (1..8) or zones (optional), pass -1 if not applicable.
// timeoutMs: scan window duration (e.g. 15000).
void rfScanArm(ScanTarget target, int8_t slot, uint32_t timeoutMs);

// Cancel an active scan session (if any).
void rfScanCancel();
