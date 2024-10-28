#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include "SemVer.h"

/**
 * @struct OtaUpdateResult_t
 * @brief Represents the result of an OTA (Over-The-Air) update attempt.
 */
struct OtaUpdateResult_t {
  bool success;    ///< Indicates whether the update was successful
  String message;  ///< Contains a message describing the result or any error
};

/**
 * @class OTAManager
 * @brief Manages Over-The-Air (OTA) updates for ESP32 devices.
 * 
 * This class provides functionality to handle firmware updates over the network.
 */
class OTAManager {
public:
  /**
     * @brief Initiates and manages the OTA update process.
     * 
     * @param url The URL of the firmware update file.
     * @param major Major version number of the new firmware.
     * @param minor Minor version number of the new firmware.
     * @param patch Patch version number of the new firmware.
     * @param forceUpdate If true, forces the update regardless of version.
     * @return OtaUpdateResult_t Struct containing the result of the update attempt.
     */
  OtaUpdateResult_t handleOTAUpdate(const String &firmwareVersion, const String &url, int major, int minor, int patch, bool forceUpdate);

private:
  /**
     * @brief Starts the actual OTA update process.
     * 
     * @param url The URL of the firmware update file.
     * @return String A message indicating the result of the update process.
     */
  String startOtaUpdate(const String &url);
};

OtaUpdateResult_t OTAManager::handleOTAUpdate(const String &firmwareVersion, const String &url, int major, int minor, int patch, bool forceUpdate) {
  OtaUpdateResult_t result = { false, "" };
  SemVer currentVersion = SemVer(firmwareVersion);
  SemVer newVersion = SemVer(String(major) + "." + String(minor) + "." + String(patch));
  bool updateAvailable = newVersion > currentVersion;

  // Log update information
  Serial.print("[OTAManager.handleOTAUpdate()]: URL: ");
  Serial.println(url.c_str());
  Serial.print("Current version: ");
  Serial.println(currentVersion.toString());
  Serial.print("New version: ");
  Serial.println(newVersion.toString());
  if (forceUpdate) Serial.println("[OTAManager.startOtaUpdate()]: Enforcing OTA update!");

  if (forceUpdate || updateAvailable) {
    if (updateAvailable) {
      Serial.println("[OTAManager.startOtaUpdate()]: Update available!");
    }
    String resp = startOtaUpdate(url);
    if (!resp.isEmpty()) {
      result.message = resp;
    } else {
      result.success = true;
    }
  } else {
    result.message = "Current version is up to date.";
  }
  return result;
}

String OTAManager::startOtaUpdate(const String &url) {
  WiFiClientSecure client;

#ifdef ENABLE_SSL_ROOT_CA_CERT_VALIDATION
  client.setCACert(OTA_CERT_CA);
#endif

  HTTPClient https;
  Serial.print("[OTAManager.startOtaUpdate()]: begin...\n");
  if (!https.begin(client, url)) return "Unable to connect";

  int httpCode = https.GET();
  if (httpCode != HTTP_CODE_OK) return "GET... failed, error: " + https.errorToString(httpCode);

  int contentLength = https.getSize();
  Serial.printf("[OTAManager.startOtaUpdate()]: OTA size: %d bytes \n", contentLength);
  if (contentLength == 0) return "There was no content length in the response";

  Serial.printf("[OTAManager.startOtaUpdate()]: Beginning update..!\n");
  bool canBegin = Update.begin(contentLength);
  if (!canBegin) return "Not enough space to begin OTA";

  WiFiClient *stream = https.getStreamPtr();
  size_t written = Update.writeStream(*stream);
  if (written != contentLength) return "Written only : " + String(written) + "/" + String(contentLength) + ". Retry?";

  Serial.println("[OTAManager.startOtaUpdate()]: startOtaUpdate()] Written : " + String(written) + " successfully");

  if (!Update.end()) return "Error Occurred. Error #: " + String(Update.getError());

  Serial.println("[OTAManager.startOtaUpdate()]: OTA done!");
  if (!Update.isFinished()) return "Update not finished? Something went wrong!";

  Serial.println("[OTAManager.startOtaUpdate()]: Update successfully completed. Rebooting.");
  ESP.restart();
  return "";
}