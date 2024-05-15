/*
 *  Copyright (c) 2019-2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro (https://github.com/sinricpro/)
 */

#pragma once 

#include <WiFi.h>
 
class WiFiUtil {
  public:
      static bool connectToWiFi();
      static bool connectToWiFi(const char * wifi_ssid, const char * wifi_password);
};

 /**
 * @brief Connect to last connected WiFi
 * @retval true
 *      Success
 * @retval false
 *      Failure
 */
bool WiFiUtil::connectToWiFi() {
  return connectToWiFi("", "");  
}
  
 /**
 * @brief Connect to WiFi with ssid, password. If not provided, use the last connected
 * @param wifi_ssid
 *      WiFi SSID
 * @param wifi_password
 *      WiFi Password
 * @retval true
 *      Success
 * @retval false
 *      Failure
 */
bool WiFiUtil::connectToWiFi(const char * wifi_ssid, const char * wifi_password) {
    #if defined(ESP32)
      #if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0))
        WiFi.setMinSecurity(WIFI_AUTH_WEP); // https://github.com/espressif/arduino-esp32/blob/master/docs/source/troubleshooting.rst
      #endif    
    #endif

    if(strlen(wifi_ssid) > 0 && strlen(wifi_password) > 0) {
      Serial.printf("[WiFiUtil.connectToWiFi()]: Got SSID:%s, PWD: %s\r\n", wifi_ssid, wifi_password);
      WiFi.persistent(true);
      WiFi.begin(wifi_ssid, wifi_password);
    } else {
      Serial.printf("[WiFiUtil.connectToWiFi()]: Connecting to WiFi...\r\n");
      WiFi.begin(); // Use credentails in NVM    
    }

    uint8_t timeout = 40 * 2; // 20 seconds
    
    while (timeout && (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0))) {
      delay(500);
      timeout--;
      Serial.printf(".");
    }
    
    Serial.printf("\r\n");  
  
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WiFiUtil.connectToWiFi()]: WiFi connected.");
      Serial.printf("IP: %s\r\n",  WiFi.localIP().toString().c_str());
      WiFi.setAutoReconnect(true);
      return true;     
    } else {
      Serial.printf("[WiFiUtil.connectToWiFi()]: WiFi connection failed. Please reboot the device and try again!\r\n");
      return false;     
    }
} 

 
  