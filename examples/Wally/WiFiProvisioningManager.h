/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 */

#pragma once

#include <SinricProBusinessSdk.h>
#include "ConfigStore.h"
#include "WiFiManager.h"

/**
 * @class WiFiProvisioningManager
 * @brief Manages WiFi provisioning for ESP32 devices.
 * 
 * This class handles the WiFi provisioning process, including connecting to WiFi
 * and configuring cloud credentials.
 */
class WiFiProvisioningManager {
public:
  /**
  * @brief Constructor for WiFiProvisioningManager.
  * @param config Reference to the ConfigStore object.
  * @param wifiManager Reference to the WiFiManager object.
  */
  WiFiProvisioningManager(ConfigStore& config, WiFiManager& wifiManager);

  /**
  * @brief Starts the WiFi provisioning process.
  * @param productId String containing the product ID.
  * @return bool True if provisioning was successful, false otherwise.
  */
  bool beginProvision(String productId);

private:
  ConfigStore& m_configStore;
  WiFiManager& m_wifiManager;

  /**
  * @brief Handles button events during provisioning.
  * @param state Current state of the button.
  */
  void handleButton(int state);

  /**
  * @brief Manages LED indicators during provisioning.
  * @param state Current state of the provisioning process.
  */
  void handleLedIndicator(int state);
};

// Constructor implementation
WiFiProvisioningManager::WiFiProvisioningManager(ConfigStore& config, WiFiManager& wifiManager)
  : m_configStore(config), m_wifiManager(wifiManager) {}

// LED indicator handling (to be implemented)
void WiFiProvisioningManager::handleLedIndicator(int state) {
  // TODO: Implement LED indicator handling based on provisioning state
  // switch (state) {
  //   case IDLE:
  //   case WAIT_WIFI_CONFIG:
  //   case WAIT_CLOUD_CONFIG:
  //   case CONNECTING_WIFI:
  //   case SUCCESS:
  //   case TIMEOUT:
  //   case ERROR:
  //     break;
  // }
}

// Button handling (to be implemented)
void WiFiProvisioningManager::handleButton(int state) {
  // TODO: Implement button handling logic
}

// Main provisioning process
bool WiFiProvisioningManager::beginProvision(String productId) {
  WiFiProv prov(productId);

  // Callback for WiFi credentials
  prov.onWiFiCredentials([this](const char* ssid, const char* password) -> bool {
    if (this->m_wifiManager.connectToWiFi(ssid, password)) {
      return this->m_wifiManager.updatePrimarySettings(ssid, password);
    }
    return false;
  });

  // Callback for cloud credentials
  prov.onCloudCredentials([this](const String& config) -> bool {
    JsonDocument jsonConfig;
    DeserializationError error = deserializeJson(jsonConfig, config);

    if (error) {
      Serial.printf("[WiFiProvisioningManager.beginProvision()]: deserializeJson() failed: %s\n", error.c_str());
      return false;
    } else {
      if (m_configStore.saveJsonConfig(jsonConfig)) {
        Serial.println(F("[WiFiProvisioningManager.beginProvision()]: Configuration updated!"));
        return true;
      } else {
        Serial.println(F("[WiFiProvisioningManager.beginProvision()]: Failed to save configuration!"));
        return false;
      }
    }
  });

  // Main provisioning loop
  prov.loop([this](int state) {
    // Handle LED indicator and button press
    this->handleLedIndicator(state);
    this->handleButton(state);
  });

  return prov.beginProvision();
}