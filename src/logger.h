#pragma once

#define ENABLE_LOG  // Comment this out to disable all logs

#ifdef ENABLE_LOG

  #include <Arduino.h>

  // Logging macros
  #define LOG_INIT()            Logger::begin()
  #define LOG_PRINT(x)          Logger::log(x)
  #define LOG_PRINTLN(x)        Logger::logln(x)
  #define LOG_LOGF(...)         Logger::logf(__VA_ARGS__)
  #define LOG_LOGF_P(...)       Logger::logf_P(__VA_ARGS__)

#else

  #define LOG_INIT()
  #define LOG_PRINT(x)
  #define LOG_PRINTLN(x)
  #define LOG_LOGF(...)
  #define LOG_LOGF_P(...)

#endif

class Logger {
public:
  static void begin(unsigned long baud = 115200) {
    Serial.begin(baud);
    while (!Serial); // Wait for Serial if needed
  }

  static void log(const char* msg) {
    Serial.print(msg);
  }

  static void log(const __FlashStringHelper* msg) {
    Serial.print(msg);
  }

  static void logln(const char* msg) {
    Serial.println(msg);
  }

  static void logln(const __FlashStringHelper* msg) {
    Serial.println(msg);
  }

  static void logf(const char* format, ...) {
    char buf[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    Serial.print(buf);
  }

  static void logf_P(const char* format_P, ...) {
    char buf[128];
    va_list args;
    va_start(args, format_P);
    vsnprintf_P(buf, sizeof(buf), format_P, args);
    va_end(args);
    Serial.print(buf);
  }
};
