/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 */

#pragma once

// #define DEBUG_PROV_LOG    1

#ifdef DEBUG_PROV_LOG
  #ifdef DEBUG_ESP_PORT
     #define DEBUG_PROV(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
  #else
    #include <Arduino.h>
    #define DEBUG_PROV(...) Serial.printf( __VA_ARGS__ )
  #endif
#else
  #define DEBUG_PROV(x...) if (false) do { (void)0; } while (0)
#endif  