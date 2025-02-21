/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 */

#pragma once

//#define ENABLE_PROV_LOG 1

#ifdef ENABLE_PROV_LOG
  #include <Arduino.h>
  #define DEBUG_PROV(...) Serial.printf( __VA_ARGS__ )
#else
  #define DEBUG_PROV(x...) if (false) do { (void)0; } while (0)
#endif  