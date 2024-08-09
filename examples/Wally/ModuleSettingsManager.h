/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 */

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "WiFiManager.h"

// Define constants for setting types
#define SET_WIFI_PRIMARY "pro.sinric::set.wifi.primary"
#define SET_WIFI_SECONDARY "pro.sinric::set.wifi.secondary"
#define SET_FIXED_IP_ADDRESS "pro.sinric::set.fixed.ip.address"

/**
 * @struct SetModuleSettingResult
 * @brief Represents the result of a module setting operation.
 */
struct SetModuleSettingResult {
    bool success;    ///< Indicates whether the operation was successful
    String message;  ///< Contains a message describing the result or any error
};

/**
 * @class ModuleSettingsManager
 * @brief Manages module settings for the ESP32 device.
 * 
 * This class provides functionality to handle various module settings,
 * particularly related to WiFi configuration.
 */
class ModuleSettingsManager { 
public:
    /**
     * @brief Constructor for ModuleSettingsManager.
     * @param wifiManager Reference to the WiFiManager object.
     */
    ModuleSettingsManager(WiFiManager &wifiManager);

    /**
     * @brief Destructor for ModuleSettingsManager.
     */
    ~ModuleSettingsManager();

    /**
     * @brief Handles setting a module configuration.
     * @param id The identifier for the setting to be changed.
     * @param value The new value for the setting.
     * @return SetModuleSettingResult Struct containing the result of the operation.
     */
    SetModuleSettingResult handleSetModuleSetting(const String& id, const String& value);

private:
    WiFiManager m_wifiManager;  ///< WiFiManager instance to handle WiFi-related operations
};