/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include "Settings.h"

/**
 * @struct OtaUpdateResult
 * @brief Represents the result of an OTA (Over-The-Air) update attempt.
 */
struct OtaUpdateResult {
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
     * @return OtaUpdateResult Struct containing the result of the update attempt.
     */
    OtaUpdateResult handleOTAUpdate(const String &url, int major, int minor, int patch, bool forceUpdate);

private:
    /**
     * @brief Starts the actual OTA update process.
     * 
     * @param url The URL of the firmware update file.
     * @return String A message indicating the result of the update process.
     */
    String startOtaUpdate(const String &url);
};