/* 
  Copyright (c) 2019-2024 Sinric
*/

#pragma once 
#include <Arduino.h>
#include <sstream>
#include "esp_system.h"
#include <WiFi.h>

class ProvUtil {
  public:
      static std::string to_string(int a);
      static uint32_t getChipId32();
      static String getMacAddress();      
      static void wait(int period);
};
