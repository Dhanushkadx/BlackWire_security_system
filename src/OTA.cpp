
#include "OTA.h"

// WiFi credentials
const char* ssid = "DeepNet"; // put your wifi name
const char* password = "dynamicocean093"; // put your wifi password
//https://github.com/Dhanushkadx/ESP32_HTTPS_OTA/releases/download/v1.2/firmware.bin
const char* firmwareUrl = "https://github.com/Dhanushkadx/ESP32_HTTPS_OTA/releases/download/v1.2/firmware.bin";
const char* versionUrl = "https://raw.githubusercontent.com/Dhanushkadx/ESP32_HTTPS_OTA/refs/heads/main/version.txt";

// Current firmware version
const char* currentFirmwareVersion = "v1.6";
const unsigned long updateCheckInterval = 5 * 60 * 1000;  // 5 minutes in milliseconds
unsigned long lastUpdateCheck = 0;


String fetchLatestVersion() {
  HTTPClient http;
  http.begin(versionUrl);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String latestVersion = http.getString();
    latestVersion.trim();  // Remove any extra whitespace
    http.end();
    return latestVersion;
  } else {
    Serial.printf("Failed to fetch version. HTTP code: %d\n", httpCode);
    http.end();
    return "";
  }
}



bool startOTAUpdate(WiFiClient* client, int contentLength) {
  Serial.println("Initializing update...");
  if (!Update.begin(contentLength)) {
    Serial.printf("Update begin failed: %s\n", Update.errorString());
    return false;
  }

  Serial.println("Writing firmware...");
  size_t written = 0;
  int progress = 0;
  int lastProgress = 0;

  // Timeout variables
  const unsigned long timeoutDuration = 120*1000;  // 10 seconds timeout
  unsigned long lastDataTime = millis();

  while (written < contentLength) {
    if (client->available()) {
      uint8_t buffer[128];
      size_t len = client->read(buffer, sizeof(buffer));
      if (len > 0) {
        Update.write(buffer, len);
        written += len;

        // Calculate and print progress
        progress = (written * 100) / contentLength;
        if (progress != lastProgress) {
          Serial.printf("Writing Progress: %d%%\n", progress);
          lastProgress = progress;
        }
      }
    }
    // Check for timeout
    if (millis() - lastDataTime > timeoutDuration) {
      Serial.println("Timeout: No data received for too long. Aborting update...");
      Update.abort();
      return false;
    }

    yield();
  }
  Serial.println("\nWriting complete");

  if (written != contentLength) {
    Serial.printf("Error: Write incomplete. Expected %d but got %d bytes\n", contentLength, written);
    Update.abort();
    return false;
  }

  if (!Update.end()) {
    Serial.printf("Error: Update end failed: %s\n", Update.errorString());
    return false;
  }

  Serial.println("Update successfully completed");
  return true;
}

void downloadAndApplyFirmware(const char* firmwareUrl) {
  Serial.println("Downloading firmware...");
  Serial.printf("Firmware URL: %s\n", firmwareUrl);
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(firmwareUrl);

  int httpCode = http.GET();
  Serial.printf("HTTP GET code: %d\n", httpCode);

  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();
    Serial.printf("Firmware size: %d bytes\n", contentLength);

    if (contentLength > 0) {
      WiFiClient* stream = http.getStreamPtr();
      if (startOTAUpdate(stream, contentLength)) {
        Serial.println("OTA update successful, restarting...");
        delay(2000);
        ESP.restart();
      } else {
        Serial.println("OTA update failed");
      }
    } else {
      Serial.println("Invalid firmware size");
    }
  } else {
    Serial.printf("Failed to fetch firmware. HTTP code: %d\n", httpCode);
  }
  http.end();
}

void checkForFirmwareUpdate() {
  Serial.println("Checking for firmware update...");
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return;
  }

  // Step 1: Fetch the latest version from GitHub
  String latestVersion = fetchLatestVersion();
  if (latestVersion == "") {
    Serial.println("Failed to fetch latest version");
    return;
  }

  Serial.println("Current Firmware Version: " + String(currentFirmwareVersion));
  Serial.println("Latest Firmware Version: " + latestVersion);

  // Step 2: Compare versions
  if (latestVersion != currentFirmwareVersion) {
    Serial.println("New firmware available. Starting OTA update...");
    downloadAndApplyFirmware(firmwareUrl);
  } else {
    Serial.println("Device is up to date.");
  }

}

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println("IP address: " + WiFi.localIP().toString());
}



void setup_http() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nStarting ESP32 OTA Update");

  connectToWiFi();
  Serial.println("Device is ready.");
  Serial.println("Current Firmware Version: " + String(currentFirmwareVersion));
  checkForFirmwareUpdate();
}


void downloadAndApplySPIFFS(const char* spiffsUrl) {
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  Serial.println("Downloading SPIFFS image...");
  http.begin(spiffsUrl);

  int httpCode = http.GET();
  Serial.printf("HTTP GET code: %d\n", httpCode);

  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();
    Serial.printf("SPIFFS image size: %d bytes\n", contentLength);

    if (contentLength > 0) {
      WiFiClient* stream = http.getStreamPtr();

      if (startSPIFFSOTAUpdate(stream, contentLength)) {
        Serial.println("SPIFFS OTA done. Rebooting...");
        delay(2000);
        ESP.restart();   // recommended
      } else {
        Serial.println("SPIFFS OTA failed");
      }
    } else {
      Serial.println("Invalid SPIFFS image size");
    }
  } else {
    Serial.printf("Failed to download SPIFFS image. HTTP code: %d\n", httpCode);
  }

  http.end();
}

bool startSPIFFSOTAUpdate(WiFiClient* client, int contentLength) {
  Serial.println("Initializing SPIFFS update...");

  if (!Update.begin(contentLength, U_SPIFFS)) {
    Serial.printf("SPIFFS Update begin failed: %s\n", Update.errorString());
    return false;
  }

  Serial.println("Writing SPIFFS image...");
  size_t written = 0;
  int progress = 0;
  int lastProgress = 0;

  const unsigned long timeoutDuration = 120 * 1000; // 2 minutes
  unsigned long lastDataTime = millis();

  while (written < contentLength) {
    if (client->available()) {
      uint8_t buffer[512];   // Bigger buffer is fine
      size_t len = client->read(buffer, sizeof(buffer));

      if (len > 0) {
        Update.write(buffer, len);
        written += len;
        lastDataTime = millis();

        progress = (written * 100) / contentLength;
        if (progress != lastProgress) {
          Serial.printf("SPIFFS Writing Progress: %d%%\n", progress);
          lastProgress = progress;
        }
      }
    }

    if (millis() - lastDataTime > timeoutDuration) {
      Serial.println("Timeout: SPIFFS OTA stalled. Aborting...");
      Update.abort();
      return false;
    }

    yield();
  }

  Serial.println("SPIFFS write complete");

  if (written != contentLength) {
    Serial.printf("SPIFFS write incomplete (%d / %d)\n", written, contentLength);
    Update.abort();
    return false;
  }

  if (!Update.end()) {
    Serial.printf("SPIFFS Update end failed: %s\n", Update.errorString());
    return false;
  }

  Serial.println("SPIFFS OTA successful");
  return true;
}


