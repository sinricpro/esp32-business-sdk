/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
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
      static void wait(uint32_t period);
};
