/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 */

#include "OTAManager.h"
#include "SemVer.h"

OtaUpdateResult OTAManager::handleOTAUpdate(const String &url, int major, int minor, int patch, bool forceUpdate) {
    OtaUpdateResult result = { false, "" };
    Version currentVersion = Version(FIRMWARE_VERSION);
    Version newVersion = Version(String(major) + "." + String(minor) + "." + String(patch));
    bool updateAvailable = newVersion > currentVersion;

    // Log update information
    Serial.print("URL: ");
    Serial.println(url.c_str());
    Serial.print("Current version: ");
    Serial.println(currentVersion.toString());
    Serial.print("New version: ");
    Serial.println(newVersion.toString());
    if (forceUpdate) Serial.println("Enforcing OTA update!");

    if (forceUpdate || updateAvailable) {
        if (updateAvailable) {
            Serial.println("Update available!");
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
    client.setCACert(rootCACertificate);
    #endif

    HTTPClient https;
    Serial.print("[startOtaUpdate()] begin...\n");
    if (!https.begin(client, url)) return "Unable to connect";

    Serial.print("[startOtaUpdate()] GET...\n");
    int httpCode = https.GET();
    if (httpCode != HTTP_CODE_OK) return "GET... failed, error: " + https.errorToString(httpCode);

    int contentLength = https.getSize();
    Serial.printf("OTA size: %d bytes \n", contentLength);
    if (contentLength == 0) return "There was no content length in the response";

    Serial.printf("Beginning update..!\n");
    bool canBegin = Update.begin(contentLength);
    if (!canBegin) return "Not enough space to begin OTA";

    WiFiClient *stream = https.getStreamPtr();
    size_t written = Update.writeStream(*stream);
    if (written != contentLength) return "Written only : " + String(written) + "/" + String(contentLength) + ". Retry?";

    Serial.println("[startOtaUpdate()] Written : " + String(written) + " successfully");
    
    if (!Update.end()) return "Error Occurred. Error #: " + String(Update.getError());
    
    Serial.println("[startOtaUpdate()] OTA done!");
    if (!Update.isFinished()) return "Update not finished? Something went wrong!";

    Serial.println("[startOtaUpdate()] Update successfully completed. Rebooting.");
    ESP.restart();
    return "";
}