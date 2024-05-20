/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 */
 
// Notes:
//  Change Tools -> Flash Size -> Minimun SPIFF
//  To enable ESP32 logs: Tools -> Core Debug Level -> Verbose

#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32) 
#else
#error "Architecture not supported!"
#endif

#define PRODUCT_ID          "66345255d495a7cbfa78445f"  // Product ID from Buiness Portal.
#define FIRMWARE_VERSION    "1.1.1"                     // Your firmware version. Must be above SinricPro.h!

#define ENABLE_DEBUG // Enable Logs.

#ifdef ENABLE_DEBUG
  #define DEBUG_ESP_PORT Serial
  #define NODEBUG_WEBSOCKETS
  #define NDEBUG
#endif

#define BAUDRATE          115200

#include <SinricProBusinessSdk.h>

#include "ConfigStore.h"
#include "WiFiUtil.h"

#include <SinricPro.h>
#include <SinricProSwitch.h>
 
DeviceConfig m_config;
ConfigStore m_configStore(m_config);
unsigned long time_now = 0;
 
bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("[main.onPowerState()]: Device: %s, power state changed to %s\r\n", deviceId.c_str(), state ? "on" : "off");  
   if(strcmp(m_config.sw1_id, deviceId.c_str()) == 0) { // is for switch 1 ?
    Serial.printf("[main.onPowerState()]: Change devie device: %s, power state changed to %s\r\n", deviceId.c_str(), state ? "on" : "off");      
  } else if(strcmp(m_config.sw2_id, deviceId.c_str()) == 0) { // is for switch 2 ?
    Serial.printf("[main.onPowerState()]: Change devie device: %s, power state changed to %s\r\n", deviceId.c_str(), state ? "on" : "off");      
  } else {
    Serial.printf("[main.onPowerState()]: Devie device: %s not found!\r\n", deviceId.c_str());  
  }
  return true; // request handled properly
}

bool onSetSetting(const String &deviceId, const String& settingId, const String& settingValue) {
  Serial.printf("[main.onSetSetting()]: Device: %s, id: %s, value: %s\r\n", deviceId.c_str(), settingId.c_str(), settingValue.c_str());
  return true; // request handled properly
}


/**
 * Setup devices.
 */
void setupSinricPro() {
  Serial.printf("[setupSinricPro()]: Setup SinricPro.\r\n");  
  SinricProSwitch& mySwitch1 = SinricPro[m_config.sw1_id];
  mySwitch1.onPowerState(onPowerState);
  mySwitch1.onSetSetting(onSetSetting);
  
  SinricProSwitch& mySwitch2 = SinricPro[m_config.sw2_id];
  mySwitch2.onPowerState(onPowerState);
  mySwitch2.onSetSetting(onSetSetting);
   
  SinricPro.onConnected([]() { Serial.printf("[main.setupSinricPro()]: Connected to SinricPro\r\n"); });
  SinricPro.onDisconnected([]() { Serial.printf("[main.setupSinricPro()]: Disconnected from SinricPro\r\n"); });
  SinricPro.begin(m_config.appKey, m_config.appSecret);
}

/**
 * Setup PINs for devices.
 */
void setupPins() {
  Serial.printf("[setupPins()]: Setup pin definition.\r\n"); 
}

void handleLedIndicator(int state) {
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

void handleButton(int state) { }

bool handleProvisioning() {
   WiFiProv prov(PRODUCT_ID);    
    
    prov.onWiFiCredentials([](const char* ssid, const char* password) -> bool {
      return WiFiUtil::connectToWiFi(ssid, password);
    });

    prov.onCloudCredentials([](const String &config) -> bool {
      DynamicJsonDocument jsonConfig(2048);
      DeserializationError error = deserializeJson(jsonConfig, config);
      
      if (error) {
        Serial.printf("[handleProvisioning()]: deserializeJson() failed: %s\r\n", error.c_str());
        return false;
      } else {
        if (m_configStore.saveJsonConfig(jsonConfig)) {
          Serial.printf("[handleProvisioning()]: Configuration updated!\r\n");
          return true;    
        }
        else {
          Serial.printf("[handleProvisioning()]: Failed to save configuration !\r\n");
          return false;
        }
      }
    });

    prov.loop([](int state) {
      // You can use loop() to handle buttons press or blink a LED.
      // handleLedIndicator(state);
      // handleButton();      
    });

    return prov.beginProvision();
}

void setup() {
  Serial.begin(BAUDRATE); Serial.println();
  delay(1000);
 
  Serial.printf("[setup()]: Firmware: %s, SinricPro SDK: %s, Business SDK:%s\r\n", FIRMWARE_VERSION, SINRICPRO_VERSION, BUSINESS_SDK_VERSION);   
  Serial.printf("[setup()]: Initialize SPIFFS...\r\n");  
  
  if (SPIFFS.begin(true)) {
    Serial.printf("[setup()]: done.\r\n");
  } else{
    Serial.printf("[setup()]: fail.\r\n");
  }

  Serial.printf("[setup()]: Setup Pins\r\n");
  setupPins();

  delay(1000); 

  bool isConfigured = m_configStore.loadConfig();
  Serial.printf("[setup()]: Provisioned ? %s\r\n", isConfigured ? "YES" : "NO");

  if(!isConfigured) {
    Serial.printf("[setup()]: Begin provisioning!\r\n");    

    if(!handleProvisioning()) {
      Serial.printf(PSTR("[setup()]: Provisioning failed. Cannot continue!.\r\n"));    
      ESP.restart();
      return;
    }
  }
  else {
    // Connect to WiFi
    while (!WiFiUtil::connectToWiFi()) { 
        Serial.printf(PSTR("[setup()]: Cannot connect to WiFi any longer. WiFi Router down? Waiting 1 min to retry.\r\n"));
        delay(60000);
        Serial.printf(PSTR("[setup()]: Trying again..."));
    }
  }
    
  Serial.printf("[setup()]: Setup SinricPro!\r\n");
  setupSinricPro();
 
  Serial.printf("[setup()]: Free Heap: %u\r\n", ESP.getFreeHeap());
  time_now = millis();
}

void loop() {
  SinricPro.handle();
  // Try to avoid calling delay() function!
     
  if(millis() > time_now + 1000){
      time_now = millis();
      Serial.printf("[loop]: Free Heap: %u\r\n", ESP.getFreeHeap()); 
  }  
}
