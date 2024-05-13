/* 
  Copyright (c) 2019-2024 Sinric
*/

#pragma once 
#include <sstream>
#include "ProvDebug.h"

#include "esp_system.h"  
#include <esp_wifi.h>
#include <mbedtls/md.h>
 
class ProvUtil {
  public:
      static bool connectToWiFi();
      static bool connectToWiFi(const char * wifi_ssid, const char * wifi_password);
      static std::string to_string(int a);
      static uint32_t getChipId32();
      static String getMacAddress();      
      static void wait(int period);
  private:
      unsigned char h2int(char c);
};

 /**
 * @brief Connect to last connected WiFi
 * @retval true
 *      Success
 * @retval false
 *      Failure
 */
bool ProvUtil::connectToWiFi() {
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
bool ProvUtil::connectToWiFi(const char * wifi_ssid, const char * wifi_password) {
    #if defined(ESP32)
      #if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0))
        WiFi.setMinSecurity(WIFI_AUTH_WEP); // https://github.com/espressif/arduino-esp32/blob/master/docs/source/troubleshooting.rst
      #endif    
    #endif

    if(strlen(wifi_ssid) > 0 && strlen(wifi_password) > 0) {
      DEBUG_PROV(PSTR("[Util.connectToWiFi()]: Got SSID:%s, PWD: %s\r\n"), wifi_ssid, wifi_password);
      WiFi.persistent(true);
      WiFi.begin(wifi_ssid, wifi_password);
    } else {
      DEBUG_PROV(PSTR("[Util.connectToWiFi()]: Connecting to WiFi...\r\n"));
      WiFi.begin(); // Use credentails in NVM    
    }

    WiFi.setAutoReconnect(true);   
    
    uint8_t timeout = 40 * 2; // 20 seconds
    
    while (timeout && (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0))) {
      delay(500);
      timeout--;
      DEBUG_PROV(".");
    }
    
    DEBUG_PROV("\r\n");  
  
    if (WiFi.status() == WL_CONNECTED) {
      DEBUG_PROV(PSTR("[Util.connectToWiFi()]: WiFi connected."));
      DEBUG_PROV(PSTR("IP: %s\r\n"),  WiFi.localIP().toString().c_str());       
      return true;     
    } else {
      DEBUG_PROV(PSTR("[Util.connectToWiFi()]: WiFi connection failed. Please reboot the device and try again!\r\n"));
      return false;     
    }
} 


/**
 * @brief Get the ESP32/8266 ChipId
 * @return uint32_t Chip Id
 */
uint32_t ProvUtil::getChipId32()
{
  uint32_t chipID = 0;

  #ifdef ESP32
    chipID = ESP.getEfuseMac() >> 16;
  #else
    chipID = ESP.getChipId();
  #endif 

  return chipID;
}

/**
 * @brief Returns the MAC address NIC
 * @return String Mac address eg: "5C:CF:7F:30:D9:9"
 */
String ProvUtil::getMacAddress() {
  return String(WiFi.macAddress());  
}

std::string ProvUtil::to_string(int a) {
   std::ostringstream ss;
   ss << a;
   return ss.str(); 
}

//wait approx. [period] ms
void ProvUtil::wait(int ms) {
  unsigned long start = millis();
  while (millis() - start < ms) delay(10);
}
  