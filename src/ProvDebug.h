/* 
  Copyright (c) 2019-2024 Sinric
*/

#pragma once

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