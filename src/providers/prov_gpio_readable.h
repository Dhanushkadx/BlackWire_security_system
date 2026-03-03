#pragma once
/**
 * @file prov_gpio_readable.h
 * @brief GPIO input provider for the BlackWire / PrimeHive alarm panel.
 *
 * What it does
 * ------------
 * - Reads N GPIO pins (channels).
 * - Converts raw pin level to an "active" level (supports inverted wiring).
 * - Detects changes vs previous sample.
 * - Emits RawInputEvent into the event bus (rawPush()) whenever a change happens.
 *
 * Typical usage
 * -------------
 *   static const GpioChannel channels[] = {
 *     { .zone = 0, .pin = 2,  .inverted = true  }, // INPUT_PULLUP -> active LOW contact
 *     { .zone = 1, .pin = 4,  .inverted = true  },
 *   };
 *
 *   ProvGPIO gpio;
 *   gpio.begin(channels, (uint8_t)(sizeof(channels)/sizeof(channels[0])));
 *   gpio.syncAll();     // optional: publish initial state at boot
 *   ...
 *   gpio.poll();        // call periodically
 */

#include <Arduino.h>
#include "../event_bus.h"  // RawInputEvent, rawPush(), RAWF_NONE

/**
 * One physical GPIO input mapped to one logical zone.
 */
struct GpioChannel {
  uint8_t zone;     ///< Logical zone id used by your system (e.g., 0..47)
  uint8_t pin;      ///< GPIO number
  bool inverted;    ///< true when using INPUT_PULLUP and contact-to-GND means "active"
};

/**
 * @brief Simple GPIO provider: change detection + event emission.
 *
 * Notes
 * -----
 * - Supports up to 32 channels per instance because it stores states in a 32-bit bitmask.
 * - "Active level" is the level AFTER applying inversion (i.e., lvl==1 means active/open/etc. as your engine expects).
 */
class ProvGPIO {
public:
  /**
   * @brief Configure GPIO channels and take the initial snapshot.
   * @param channels Pointer to channel array (must remain valid after begin()).
   * @param channelCount Number of channels (<= 32).
   */
  void begin(const GpioChannel* channels, uint8_t channelCount);

  /**
   * @brief Poll pins, detect changes, and push RawInputEvent(s) for any changed channels.
   * Call periodically from your polling task/loop.
   */
  void poll();

  /**
   * @brief Publish the current state for ALL channels immediately (useful at boot).
   * Also updates the internal previous state mask so subsequent poll() works correctly.
   */
  void syncAll();

private:
  // Non-owning pointer to caller-provided channel table.
  const GpioChannel* channels_ = nullptr;

  // Number of channels in channels_.
  uint8_t channelCount_ = 0;

  // Previous "active level" snapshot (bit i == level of channels_[i]).
  uint32_t previousLevelMask_ = 0;

private:
  /**
   * @brief Read a channel and return its "active level" (after inversion).
   */
  inline bool readActiveLevel_(uint8_t index) const {
    const bool raw = digitalRead(channels_[index].pin);
    return channels_[index].inverted ? !raw : raw;
  }

  /**
   * @brief Configure pinMode for a channel based on inversion.
   */
  inline void configurePinMode_(uint8_t index) const {
    pinMode(channels_[index].pin, channels_[index].inverted ? INPUT_PULLUP : INPUT);
  }
};
