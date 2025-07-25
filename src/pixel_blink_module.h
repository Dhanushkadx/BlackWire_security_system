// PixelBlink.h - Header file for PixelBlink LED control class
// Supports blinking, pulsing, and steady light modes with Adafruit NeoPixel

#ifndef PIXELBLINK_H
#define PIXELBLINK_H

#include <Adafruit_NeoPixel.h>

// Available LED operation modes
enum PixelMode {
    MODE_OFF,      // LED is off
    MODE_BLINK,    // LED blinks with on/off time
    MODE_PULSE,    // LED fades in/out smoothly
    MODE_STEADY    // LED stays on continuously
};

class PixelBlink {
public:
    // Constructor: pin number and LED count (usually 1)
    PixelBlink(uint8_t pin, uint8_t count);

    // Initializes the NeoPixel library
    void begin();

    // Call this in loop() to manage blinking/pulsing/steady behavior
    void update();

    // Start blinking: color, on/off time, brightness, and optional timeout in ms
    void startBlink(uint32_t color, uint16_t onTime, uint16_t offTime, uint8_t brightness = 255, uint32_t timeout = 0);

    // Start pulsing (fade in/out): color, pulse time (full cycle), and max brightness
    void startPulse(uint32_t color, uint16_t pulseTime = 1000, uint8_t maxBrightness = 255);

    // Turn on LED in steady mode
    void startSteady(uint32_t color, uint8_t brightness = 255);

    // Turn off LED and stop all behavior
    void stop();

    // Returns true if any mode is active
    bool isActive() const;

    // Returns current LED mode
    PixelMode getMode() const;

private:
    Adafruit_NeoPixel pixel;     // NeoPixel object
    uint8_t _pin, _count;        // LED pin and count

    PixelMode _mode;             // Current LED mode
    uint32_t _color;             // LED color
    uint16_t _onTime;            // ON duration (for blink)
    uint16_t _offTime;           // OFF duration (for blink)
    uint8_t _brightness;         // Fixed brightness (steady or blink)
    uint8_t _maxBrightness;      // Max brightness (pulse)
    uint32_t _timeout;           // Optional auto-stop timeout (ms)

    unsigned long _previousMillis; // For timing blink
    unsigned long _startMillis;    // For timeout or pulse cycle
    bool _ledState;                // LED state for blink
};

#endif
