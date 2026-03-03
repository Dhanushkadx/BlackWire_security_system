/**
 * @file prov_gpio_readable.cpp
 * @brief Implementation of ProvGPIO.
 */

#include "prov_gpio_readable.h"

// Optional debug prints (uncomment to enable)
// #define PROVGPIO_DEBUG 1

void ProvGPIO::begin(const GpioChannel* channels, uint8_t channelCount) {
  channels_ = channels;
  channelCount_ = channelCount;
  previousLevelMask_ = 0;

  if (!channels_ || channelCount_ == 0) {
    return;
  }

  // Protect against >32 channels (mask is 32-bit).
  if (channelCount_ > 32) {
    channelCount_ = 32;
  }

  // Configure pins + build initial snapshot.
  for (uint8_t i = 0; i < channelCount_; i++) {
    configurePinMode_(i);

    const bool active = readActiveLevel_(i);
    if (active) {
      previousLevelMask_ |= (1UL << i);
    }
  }
}

void ProvGPIO::poll() {
  if (!channels_ || channelCount_ == 0) {
    return;
  }

  // Build current snapshot.
  uint32_t currentLevelMask = 0;
  for (uint8_t i = 0; i < channelCount_; i++) {
    if (readActiveLevel_(i)) {
      currentLevelMask |= (1UL << i);
    }
  }

  // Which bits changed since last time?
  uint32_t changedMask = currentLevelMask ^ previousLevelMask_;
  if (changedMask == 0) {
    return; // nothing changed
  }

  // Update snapshot first (so even if rawPush triggers other logic, our state is consistent).
  previousLevelMask_ = currentLevelMask;

  // Emit an event for each changed channel.
  while (changedMask) {
    // Extract least significant '1' bit (LSB).
    const uint32_t lsb = changedMask & (uint32_t)(-(int32_t)changedMask);
    const uint8_t index = (uint8_t)__builtin_ctz(changedMask);
    changedMask ^= lsb;

    const uint8_t level = (uint8_t)((currentLevelMask >> index) & 0x01);

#ifdef PROVGPIO_DEBUG
    Serial.printf("GPIO change: ch=%u zone=%u level=%u\n", index, channels_[index].zone, level);
#endif

    rawPush(RawInputEvent{ channels_[index].zone, level, RAWF_NONE, millis() });
  }
}

void ProvGPIO::syncAll() {
  if (!channels_ || channelCount_ == 0) {
    return;
  }

  const uint32_t now = millis();
  uint32_t currentLevelMask = 0;

  // Read each channel and push its current state.
  for (uint8_t i = 0; i < channelCount_; i++) {
    const bool active = readActiveLevel_(i);
    if (active) {
      currentLevelMask |= (1UL << i);
    }

#ifdef PROVGPIO_DEBUG
    Serial.printf("GPIO sync: ch=%u zone=%u level=%u\n", i, channels_[i].zone, (uint8_t)active);
#endif

    rawPush(RawInputEvent{ channels_[i].zone, (uint8_t)active, RAWF_NONE, now });
  }

  // Important: update snapshot so next poll() detects changes correctly.
  previousLevelMask_ = currentLevelMask;
}
