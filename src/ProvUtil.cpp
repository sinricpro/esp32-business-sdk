/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 */

#include "ProvUtil.h"
 
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
  