# 🔐 PrimeHive Security System

## Overview

PrimeHive Security System is a modular, embedded-based intrusion detection platform built on ESP32-S3.  
It is designed for residential and small commercial security applications with real-time zone monitoring, configurable alarm logic, and cloud connectivity support.

The system architecture separates configuration management from real-time processing to ensure deterministic behavior, scalability, and maintainability.

---

## Firmware update link format
const char* firmwareUrl = "https://github.com/Dhanushkadx/ESP32_HTTPS_OTA/releases/download/v1.2/firmware.bin";

## spiffs update link format
const char* versionUrl = "https://raw.githubusercontent.com/Dhanushkadx/ESP32_HTTPS_OTA/refs/heads/main/version.txt";

## MQTT topic for firmware update command
sprintf_P(my_topic, PSTR("blackwire/%s/cmd/sys/ota/firmware"), device_id_macStr);

## MQTT topic for spiffs update command

