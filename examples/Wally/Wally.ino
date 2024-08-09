/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the SinricPro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 */

/**
 * @brief Example for our two-channel relay product (Wally) using Sinric Pro ESP32 Business SDK
 * 
 * This example demonstrates how to set up and use Wally product
 * with the Sinric Pro ESP32 Business SDK. It includes WiFi provisioning,
 * OTA updates, health monitoring, and device control.
 * 
 * @note This code supports ESP32 only.
 * @note Change Tools -> Flash Size -> Minimum SPIFF
 * @note To enable ESP32 logs: Tools -> Core Debug Level -> Verbose
 * @note Install NimBLE, ArduinoJson (v7) from library manager.
 */

#include <Arduino.h>
#include <SinricProBusinessSdk.h>

#include "Settings.h"
#include "ConfigStore.h"
#include "WiFiManager.h"
#include "WiFiProvisioningManager.h"
#include "OTAManager.h"
#include "ModuleSettingsManager.h"
#include "HealthManager.h"

#include "SinricPro.h"
#include "SinricProSwitch.h"

// Global variables and objects
DeviceConfig m_config;
ConfigStore m_configStore(m_config);
WiFiManager m_wifiManager(WIFI_CONFIG_FILE_NAME);
WiFiProvisioningManager m_provisioningManager(m_configStore, m_wifiManager);
ModuleSettingsManager m_moduleSettingsManager(m_wifiManager);
OTAManager m_otaManager;
HealthManager m_healthManager;
unsigned long m_lastHeartbeatMills = 0;

/**
 * @brief Callback function for power state changes
 * 
 * This function is called when a power state change is requested for a switch.
 * 
 * @param deviceId The ID of the device
 * @param state The new power state (true for on, false for off)
 * @return true if the request was handled properly
 */
bool onPowerState(const String& deviceId, bool& state) {
  if (strcmp(m_config.switch_1, deviceId.c_str()) == 0) {
    Serial.printf("[main.onPowerState()]: Change device: %s, power state changed to %s\r\n", deviceId.c_str(), state ? "on" : "off");
    // TODO: Implement the actual control of switch 1
  } else if (strcmp(m_config.switch_2, deviceId.c_str()) == 0) {
    Serial.printf("[main.onPowerState()]: Change device: %s, power state changed to %s\r\n", deviceId.c_str(), state ? "on" : "off");
    // TODO: Implement the actual control of switch 2
  } else {
    Serial.printf("[main.onPowerState()]: Device: %s not found!\r\n", deviceId.c_str());
  }

  return true;
}

/**
 * @brief Callback function for setting module settings
 * 
 * This function is called when a module setting change is requested.
 * 
 * @param id The ID of the setting
 * @param value The new value for the setting
 * @return true if the setting was changed successfully
 */
bool onSetModuleSetting(const String& id, const String& value) {
  SetModuleSettingResult result = m_moduleSettingsManager.handleSetModuleSetting(id, value);
  if (!result.success) {
    SinricPro.setResponseMessage(std::move(result.message));
  }
  return result.success;
}

/**
 * @brief Callback function for OTA updates
 * 
 * This function is called when an OTA update is requested.
 * 
 * @param url The URL of the update
 * @param major Major version number
 * @param minor Minor version number
 * @param patch Patch version number
 * @param forceUpdate Whether to force the update
 * @return true if the update was successful
 */
bool onOTAUpdate(const String& url, int major, int minor, int patch, bool forceUpdate) {
  OtaUpdateResult result = m_otaManager.handleOTAUpdate(url, major, minor, patch, forceUpdate);
  if (!result.success) {
    SinricPro.setResponseMessage(std::move(result.message));
  }
  return result.success;
}

/**
 * @brief Set up SinricPro
 * 
 * This function initializes SinricPro and sets up the necessary callbacks.
 */
void setupSinricPro() {
  Serial.printf("[setupSinricPro()]: Setup SinricPro.\r\n");

  SinricProSwitch& mySwitch1 = SinricPro[m_config.switch_1];
  mySwitch1.onPowerState(onPowerState);

  SinricProSwitch& mySwitch2 = SinricPro[m_config.switch_2];
  mySwitch2.onPowerState(onPowerState);

  SinricPro.onConnected([]() {
    Serial.printf("[setupSinricPro()]: Connected to SinricPro\r\n");
  });

  SinricPro.onDisconnected([]() {
    Serial.printf("[setupSinricPro()]: Disconnected from SinricPro\r\n");
  });

  SinricPro.onPong([](uint32_t since) {
    m_lastHeartbeatMills = millis();
  });

  SinricPro.onReportHealth([&](String& healthReport) {
    return m_healthManager.reportHealth(healthReport);
  });

  SinricPro.onSetSetting(onSetModuleSetting);
  SinricPro.onOTAUpdate(onOTAUpdate);

  SinricPro.begin(m_config.appKey, m_config.appSecret);
}

/**
 * @brief Set up GPIO pins for devices
 * 
 * This function should be implemented to set up any necessary GPIO pins.
 */
void setupPins() {
  Serial.printf("[setupPins()]: Setup pin definition.\r\n");
  // TODO: Implement GPIO setup for your specific hardware
}

/**
 * @brief Set up LittleFS file system
 * 
 * This function initializes the LittleFS file system.
 */
void setupLittleFS() {
  if (LittleFS.begin(true)) {
    Serial.printf("[setupLittleFS()]: done.\r\n");
  } else {
    Serial.printf("[setupLittleFS()]: fail.\r\n");
  }
}

/**
 * @brief Handle no heartbeat situation
 * 
 * This function checks if there's been no heartbeat from the server for a specified interval,
 * and resets the ESP32 if necessary.
 */
void handleNoHeartbeat() {
  unsigned long currentMillis = millis();
  if (currentMillis - m_lastHeartbeatMills >= NO_HEART_BEAT_RESET_INTERVAL) {
    Serial.println("No heartbeat for 15 mins. Restarting ESP32...");
    ESP.restart();
  }
}

/**
 * @brief Load configuration and set up WiFi
 * 
 * This function loads the device configuration and sets up the WiFi connection.
 * If the device is not provisioned, it begins the provisioning process.
 */
void loadConfigAndSetupWiFi() {
  Serial.printf("[loadConfigAndSetupWiFi()]: Loading config...\r\n");

  if (m_configStore.loadConfig()) {
    Serial.printf("[loadConfigAndSetupWiFi()]: Connecting to WiFi...\r\n");
    m_wifiManager.loadConfig();

    unsigned long startMillis = millis();
    const int timeout = 1000 * 60 * 10;  // reset in 10 mins

    while (!m_wifiManager.connectToWiFi()) {
      Serial.printf("[loadConfigAndSetupWiFi()]: Cannot connect to WiFi. Retry in 30 seconds!\r\n");
      delay(30000);
      Serial.printf("[loadConfigAndSetupWiFi()]: Attempting reconnection...\r\n");

      if ((millis() - startMillis) > timeout) {
        Serial.printf("[loadConfigAndSetupWiFi()]: Connection retry timeout. Restarting ESP...\r\n");
        ESP.restart();
      }
    }
  } else {
    Serial.printf("[loadConfigAndSetupWiFi()]: Beginning provisioning...\r\n");
    if (!m_provisioningManager.beginProvision(PRODUCT_ID)) {
      Serial.printf("[loadConfigAndSetupWiFi()]: Provisioning failed. Restarting device.\r\n");
      ESP.restart();
    }
  }
}

void setup() {
  Serial.begin(BAUDRATE);
  Serial.println();
  delay(1000);

  Serial.printf(PSTR("[setup()]: Firmware: %s, SinricPro SDK: %s, Business SDK:%s\r\n"), FIRMWARE_VERSION, SINRICPRO_VERSION, BUSINESS_SDK_VERSION);

  Serial.printf("[setup()]: Setup LittleFS...\r\n");
  setupLittleFS();

  Serial.printf("[setup()]: Setup GPIO Pins\r\n");
  setupPins();

  Serial.printf("[setup()]: Setup config\r\n");
  loadConfigAndSetupWiFi();

  Serial.printf("[setup()]: Setup SinricPro!\r\n");
  setupSinricPro();
}

void loop() {
  SinricPro.handle();
  handleNoHeartbeat();
  // Note: Avoid using delay() in the loop. Use non-blocking techniques for timing.
}