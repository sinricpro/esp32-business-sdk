/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 */

//  ~~ SUPPORTS ESP32 ONLY ~~
// THIS IS AN EXAMPLE FOR TWO CHANNEL RELAY PRODUCT
 
// Notes:
//  Change Tools -> Flash Size -> Minimun SPIFF
//  To enable ESP32 logs: Tools -> Core Debug Level -> Verbose
//  Install NimBLE, ArduinoJson (v7)  from library manager.

#include <Arduino.h>

#define PRODUCT_ID          ""  // Product ID from Buiness Portal.
#define FIRMWARE_VERSION    "1.1.1"       // Your firmware version. Must be above SinricPro.h!

#define ENABLE_DEBUG // Enable Logs.

#ifdef ENABLE_DEBUG
  #define DEBUG_ESP_PORT Serial
  #define NODEBUG_WEBSOCKETS
  #define NDEBUG
#endif

#define BAUDRATE          115200

#include <SinricProBusinessSdk.h>

#include "ConfigStore.h"
#include "WiFiManager.h"
#include "WiFiProvisioningManager.h"

#include "SinricPro.h"
#include "SinricProSwitch.h"
 
DeviceConfig m_config;
ConfigStore m_configStore(m_config);
WiFiProvisioningManager provisioningManager(m_configStore);
 
bool onPowerState(const String &deviceId, bool &state) {
    if(strcmp(m_config.switch_1, deviceId.c_str()) == 0) { // is for switch 1 ?
      Serial.printf("[main.onPowerState()]: Change devie device: %s, power state changed to %s\r\n", deviceId.c_str(), state ? "on" : "off");      
    } else if(strcmp(m_config.switch_2, deviceId.c_str()) == 0) { // is for switch 2 ?
      Serial.printf("[main.onPowerState()]: Change devie device: %s, power state changed to %s\r\n", deviceId.c_str(), state ? "on" : "off");      
    } else {
      Serial.printf("[main.onPowerState()]: Devie device: %s not found!\r\n", deviceId.c_str());  
    }

    return true; // request handled properly
}

/**
 * @brief Setup devices.
 */
void setupSinricPro() {
  Serial.printf("[setupSinricPro()]: Setup SinricPro.\r\n");  
    
  SinricProSwitch &mySwitch1 = SinricPro[m_config.switch_1];
  mySwitch1.onPowerState(onPowerState);

  SinricProSwitch &mySwitch2 = SinricPro[m_config.switch_2];
  mySwitch2.onPowerState(onPowerState);
    
  SinricPro.onConnected([]() { Serial.printf("[setupSinricPro()]: Connected to SinricPro\r\n"); });
  SinricPro.onDisconnected([]() { Serial.printf("[setupSinricPro()]: Disconnected from SinricPro\r\n"); });
  SinricPro.begin(m_config.appKey, m_config.appSecret);
}

/**
 * @brief Setup GPIO pins for your devices.
 */
void setupPins() {
  Serial.printf("[setupPins()]: Setup pin definition.\r\n"); 
}

/**
 * @brief Setup LittleFS file system.
 */
void setupLittleFS() {
  if (LittleFS.begin(true)) {
    Serial.printf("[setupLittleFS()]: done.\r\n");
  } else {
    Serial.printf("[setupLittleFS()]: fail.\r\n");
  }
}

/**
 * @brief Load device configuration. If not provisioned, begin provisioning.
 */
void loadConfigAndSetupWiFi() {
  if (m_configStore.loadConfig()) {
      Serial.printf("[loadConfigAndSetupWiFi()]: Connecting to WiFi...\r\n");
      
      while (!WiFiManager::connectToWiFi()) { 
          Serial.printf("[loadConfigAndSetupWiFi()]: Cannot connect to WiFi. Router down? Retrying in 1 minute.\r\n");
          delay(60000);
          Serial.printf("[loadConfigAndSetupWiFi()]: Attempting reconnection...\r\n");
      }
  } else {
      Serial.printf("[loadConfigAndSetupWiFi()]: Beginning provisioning...\r\n");    
      if (!provisioningManager.beginProvision(PRODUCT_ID)) {
          Serial.printf("[loadConfigAndSetupWiFi()]: Provisioning failed. Restarting device.\r\n");    
          ESP.restart();
      }
  }
}

void setup() {
  Serial.begin(BAUDRATE); Serial.println();
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
  // Try to avoid calling delay() function! 
}