/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 */

#pragma once

#include <ArduinoJson.h>
#include <Preferences.h>
#include "SPIFFS.h"

/**
 * @struct DeviceConfig
 * @brief Holds the configuration data for the device.
 * 
 * If you need security, please save this to NVS
 */
struct DeviceConfig {
  char appKey[38];     ///< Application key
  char appSecret[76];  ///< Application secret
  
  char switch_1_id[26];   ///< ID for switch 1
  char switch_1_name[32]; ///< Name for switch 1

  char switch_2_id[26];   ///< ID for switch 2
  char switch_2_name[32]; ///< Name for switch 2
};

/**
 * @class ConfigStore
 * @brief Manages the loading, saving, and clearing of device configuration.
 */
class ConfigStore {
public:
  /**
     * @brief Constructor for ConfigStore.
     * @param config Reference to a DeviceConfig object to store the configuration.
     */
  ConfigStore(DeviceConfig &config);

  /**
     * @brief Destructor for ConfigStore.
     */
  ~ConfigStore();

  /**
     * @brief Loads the configuration from the file system.
     * @return bool True if loading was successful, false otherwise.
     */
  bool loadConfig();

  /**
     * @brief Saves the configuration to the file system.
     * @param doc JsonDocument containing the configuration to save.
     * @return bool True if saving was successful, false otherwise.
     */
  bool saveJsonConfig(const JsonDocument &doc);

  /**
     * @brief Clears the configuration from the file system and memory.
     * @return bool True if clearing was successful, false otherwise.
     */
  bool clear();

private:
  DeviceConfig &config;     ///< Reference to the DeviceConfig object
  Preferences preferences;  ///< Preferences object for storing data
};

ConfigStore::ConfigStore(DeviceConfig &config)
  : config(config) {}
ConfigStore::~ConfigStore() {}

bool ConfigStore::loadConfig() {
  Serial.printf("[ConfigStore.loadConfig()]: Loading config...\r\n");

  File configFile = SPIFFS.open(PRODUCT_CONFIG_FILE, "r");
  if (!configFile) {
    Serial.printf("[ConfigStore.loadConfig()]: Config file does not exist! New device?\r\n");
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, configFile);

  if (err) {
    #ifdef ENABLE_DEBUG
        Serial.print("File size: ");
        Serial.println(configFile.size());
        Serial.print("File contents: ");
        while (configFile.available()) {
          Serial.write(configFile.read());
        }
        Serial.println();
    #endif

    configFile.close();
    Serial.printf("[ConfigStore.loadConfig()]: deserializeJson() failed: %s\r\n", err.c_str());
    return false;
  }

#ifdef ENABLE_DEBUG
  serializeJsonPretty(doc, Serial);
#endif

  // Copy configuration data from JSON to config struct
  strlcpy(config.appKey, doc[F("credentials")][F("appkey")] | "", sizeof(config.appKey));
  strlcpy(config.appSecret, doc[F("credentials")][F("appsecret")] | "", sizeof(config.appSecret));

  strlcpy(config.switch_1_id, doc[F("devices")][0][F("id")] | "", sizeof(config.switch_1_id));
  strlcpy(config.switch_1_name, doc[F("devices")][0][F("name")] | "", sizeof(config.switch_1_name));

  strlcpy(config.switch_2_id, doc[F("devices")][1][F("id")] | "", sizeof(config.switch_2_id));
  strlcpy(config.switch_2_name, doc[F("devices")][1][F("name")] | "", sizeof(config.switch_2_name));

  Serial.printf("success!\r\n");
  doc.clear();
  configFile.close();
  return true;
}

bool ConfigStore::saveJsonConfig(const JsonDocument &doc) {
  Serial.printf("[ConfigStore.saveJsonConfig()]: Saving config...\r\n");

  String appKey = doc[F("credentials")][F("appkey")] | "";
  String appSecret = doc[F("credentials")][F("appsecret")] | "";

  if (appKey.length() == 0 || appSecret.length() == 0) {
    Serial.printf("[ConfigStore.saveJsonConfig()]: Failed! Invalid configurations!\r\n");
    return false;
  }

  Serial.printf("[ConfigStore.saveJsonConfig()]: config: \r\n");
  serializeJsonPretty(doc, Serial);

  // Remove existing config file if it exists
  if (SPIFFS.exists(PRODUCT_CONFIG_FILE)) {
    Serial.printf("[ConfigStore.saveJsonConfig()]: Removing existing config file..\r\n");
    SPIFFS.remove(PRODUCT_CONFIG_FILE);
  }

  File configFile = SPIFFS.open(PRODUCT_CONFIG_FILE, FILE_WRITE);

  if (!configFile) {
    Serial.printf("[ConfigStore.saveJsonConfig] Open config file failed!!!\r\n");
    return false;
  }

  // Write JSON to file
  String jsonStr;
  serializeJson(doc, jsonStr);
  size_t bytesWritten = configFile.print(jsonStr);
  configFile.close();

  Serial.printf("[ConfigStore.saveJsonConfig] Bytes written: %u\r\n", bytesWritten);

  // Update config struct with new values
  strlcpy(config.appKey, appKey.c_str(), sizeof(config.appKey));
  strlcpy(config.appSecret, appSecret.c_str(), sizeof(config.appSecret));
  
  strlcpy(config.switch_1_id, doc[F("devices")][0][F("id")] | "", sizeof(config.switch_1_id));
  strlcpy(config.switch_1_name, doc[F("devices")][0][F("name")] | "", sizeof(config.switch_1_name));

  strlcpy(config.switch_2_id, doc[F("devices")][1][F("id")] | "", sizeof(config.switch_2_id));
  strlcpy(config.switch_2_name, doc[F("devices")][1][F("name")] | "", sizeof(config.switch_2_name));

  Serial.printf("[ConfigStore.saveJsonConfig()]: success!\r\n");

  return true;
}

bool ConfigStore::clear() {
  Serial.printf("[ConfigStore.clear()]: Clear config...");

  // Remove config file from file system
  SPIFFS.begin();
  SPIFFS.remove(PRODUCT_CONFIG_FILE);
  SPIFFS.end();

  // Clear config struct
  memset(config.appKey, 0, sizeof(config.appKey));
  memset(config.appSecret, 0, sizeof(config.appSecret));

  memset(config.switch_1_id, 0, sizeof(config.switch_1_id));
  memset(config.switch_2_name, 0, sizeof(config.switch_2_name));

  memset(config.switch_2_id, 0, sizeof(config.switch_2_id));
  memset(config.switch_2_name, 0, sizeof(config.switch_2_name));

  Serial.printf("Done...\r\n");
  return true;
}