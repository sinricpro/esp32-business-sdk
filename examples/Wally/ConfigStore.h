/*
 *  Copyright (c) 2019-2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro (https://github.com/sinricpro/)
 */

// TODO: Save to NVS

#pragma once 

#include <ArduinoJson.h>
#include "Settings.h"
#include "nvs.h"
#include "nvs_flash.h"

struct DeviceConfig
{
  char appKey[38];
  char appSecret[76];
  char sw1_id[26];
  char sw2_id[26];
};

class ConfigStore {
  public:
    ConfigStore(DeviceConfig &config);
    ~ConfigStore();
    
    bool loadConfig();
    bool saveJsonConfig(const JsonDocument &doc);
    bool clear();
    
  private:
    DeviceConfig &config;
    
    #ifdef ESP32
      Preferences preferences;
    #endif
};

ConfigStore::ConfigStore(DeviceConfig &config) : config(config) { }
ConfigStore::~ConfigStore(){ }

/**
* @brief Load the product configurations
* @return
*      ok
*/  
bool ConfigStore::loadConfig() {
  Serial.printf("[ConfigStore.loadConfig()] Loading config...\r\n");

  // TODO store in NVS.
  
  File configFile = SPIFFS.open(PRODUCT_CONFIG_FILE, "r");
  if (!configFile) {
    Serial.printf("[ConfigStore.loadConfig()]: Config file does not exists! New device?\r\n");
    return false;
  }

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, configFile);

  if (err) {
    #ifdef DEBUG_PROV_LOG
      Serial.print("File size: ");
      Serial.println(configFile.size());

      Serial.print("File contents: "); 
      while (configFile.available()) { 
        Serial.write(configFile.read());
      }
    #endif
          
    configFile.close();
    Serial.printf("[ConfigStore.loadConfig()]: deserializeJson() failed: %s\r\n", err.c_str());
    return false;
  }

  #ifdef DEBUG_PROV_LOG
    serializeJsonPretty(doc, Serial);
  #endif
    
  strlcpy(config.appKey, doc[F("credentials")][F("appkey")] | "", sizeof(config.appKey));
  strlcpy(config.appSecret, doc[F("credentials")][F("appsecret")] | "", sizeof(config.appSecret));
  strlcpy(config.sw1_id, doc[F("devices")][0][F("id")] | "", sizeof(config.sw1_id));
  strlcpy(config.sw2_id, doc[F("devices")][1][F("id")] | "", sizeof(config.sw2_id));  
    
  Serial.printf("success!\r\n");
  doc.clear();
  configFile.close();
  return true;
}

/**
* @brief Save Json config to file
* @return
*      ok
*/ 
bool ConfigStore::saveJsonConfig(const JsonDocument &doc){
  Serial.printf("[ConfigStore.saveJsonConfig()]: Saving config...\r\n"); 
   
  String appKey = doc[F("credentials")][F("appkey")] | "";
  String appSecret =  doc[F("credentials")][F("appsecret")] | "";
  
  if(appKey.length() == 0 || appSecret.length() == 0) {
    Serial.printf("[ConfigStore.saveJsonConfig()]: Failed!. Invalid configurations!\r\n");
    return false;
  }
 
  Serial.printf("[ConfigStore.saveJsonConfig()]: config: \r\n"); 
  serializeJsonPretty(doc, Serial);

  if(SPIFFS.exists(PRODUCT_CONFIG_FILE)) {
    Serial.printf("[ConfigStore.saveJsonConfig()]: Removing existing config file..\r\n"); 
    SPIFFS.remove(PRODUCT_CONFIG_FILE);
  }

  File configFile = SPIFFS.open(PRODUCT_CONFIG_FILE, "w");

  if (!configFile) {
    Serial.printf("[ConfigStore.saveJsonConfig] Open config file failed!!!\r\n"); 
    return false;
  }
 
  //size_t bytesWritten = serializeJson(doc, configFile);
  String jsonStr;
  serializeJson(doc, jsonStr);
  size_t bytesWritten = configFile.print(jsonStr);
  configFile.close();

  Serial.printf("[ConfigStore.saveJsonConfig] Bytes written: %u\r\n", bytesWritten); 
  
  strlcpy(config.appKey, appKey.c_str(), sizeof(config.appKey));
  strlcpy(config.appSecret, appSecret.c_str(), sizeof(config.appSecret));
  strlcpy(config.sw1_id, doc[F("devices")][0][F("id")] | "", sizeof(config.sw1_id));
  strlcpy(config.sw2_id, doc[F("devices")][1][F("id")] | "", sizeof(config.sw2_id));
    
  Serial.printf("[ConfigStore.saveJsonConfig()]: success!\r\n"); 
  
  return true;
}

/**
* @brief Clear stored configuations
* @return
*      ok
*/ 
bool ConfigStore::clear(){
  Serial.printf("[ConfigStore.clear()]: Clear config..."); 
   
  SPIFFS.begin();
  SPIFFS.remove(PRODUCT_CONFIG_FILE);
  SPIFFS.end();

  memset(config.appKey, 0, sizeof(config.appKey));
  memset(config.appSecret, 0, sizeof(config.appSecret));
  memset(config.sw1_id, 0, sizeof(config.sw1_id));
  memset(config.sw2_id, 0, sizeof(config.sw2_id));
 
  Serial.printf("Done...\r\n"); 
  return true;
}
