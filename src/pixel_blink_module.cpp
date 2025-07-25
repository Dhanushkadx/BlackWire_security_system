// PixelBlink.cpp - Implementation of PixelBlink class for LED control
// Supports blinking, pulsing, and steady modes using Adafruit NeoPixel

#include "pixel_blink_module.h"
#include <math.h> // Required for sin()

// Constructor
PixelBlink::PixelBlink(uint8_t pin, uint8_t count)
    : pixel(count, pin, NEO_GRB + NEO_KHZ800),
      _pin(pin), _count(count),
      _mode(MODE_OFF),
      _color(0), _onTime(500), _offTime(500),
      _brightness(255), _maxBrightness(255),
      _timeout(0), _previousMillis(0), _startMillis(0), _ledState(false)
{}

// Initialize NeoPixel
void PixelBlink::begin() {
    pixel.begin();
    pixel.setBrightness(_brightness);
    pixel.clear();
    pixel.show();
}

// Start blinking mode
void PixelBlink::startBlink(uint32_t color, uint16_t onTime, uint16_t offTime, uint8_t brightness, uint32_t timeout) {
    _mode = MODE_BLINK;
    _color = color;
    _onTime = onTime;
    _offTime = offTime;
    _brightness = brightness;
    _timeout = timeout;
    _startMillis = millis();
    _previousMillis = millis();
    _ledState = false;
    pixel.setBrightness(_brightness);
    pixel.clear();
    pixel.show();
}

// Start pulse mode (smooth fade in/out)
void PixelBlink::startPulse(uint32_t color, uint16_t pulseTime, uint8_t maxBrightness) {
    _mode = MODE_PULSE;
    _color = color;
    _onTime = pulseTime;
    _maxBrightness = maxBrightness;
    _startMillis = millis();
    _previousMillis = millis();
    pixel.clear();
    pixel.show();
}

// Start steady (always on) mode
void PixelBlink::startSteady(uint32_t color, uint8_t brightness) {
    _mode = MODE_STEADY;
    _color = color;
    _brightness = brightness;
    pixel.setBrightness(_brightness);
    pixel.setPixelColor(0, _color);
    pixel.show();
}

// Stop all LED activity
void PixelBlink::stop() {
    _mode = MODE_OFF;
    pixel.clear();
    pixel.show();
}

// Check if LED is active
bool PixelBlink::isActive() const {
    return _mode != MODE_OFF;
}

// Get the current mode
PixelMode PixelBlink::getMode() const {
    return _mode;
}

// Update LED status based on selected mode
void PixelBlink::update() {
    unsigned long currentMillis = millis();

    if (_mode == MODE_OFF) return;

    if (_mode == MODE_BLINK) {
        if (_timeout > 0 && (currentMillis - _startMillis >= _timeout)) {
            stop();
            return;
        }

        uint16_t interval = _ledState ? _onTime : _offTime;

        if (currentMillis - _previousMillis >= interval) {
            _previousMillis = currentMillis;
            _ledState = !_ledState;

            if (_ledState) {
                pixel.setPixelColor(0, _color);
            } else {
                pixel.clear();
            }

            pixel.setBrightness(_brightness);
            pixel.show();
        }
    }

    else if (_mode == MODE_PULSE) {
        float progress = (float)(currentMillis - _startMillis) / _onTime;
        float brightness = 0.5f * (1.0f + sin(2 * PI * progress));  // Sine wave fade
        pixel.setBrightness((uint8_t)(_maxBrightness * brightness));
        pixel.setPixelColor(0, _color);
        pixel.show();

        if (progress >= 1.0f) {
            _startMillis = currentMillis;
        }
    }
}
