/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 */

#pragma once 

#define IDLE              0
#define WAIT_WIFI_CONFIG  1
#define CONNECTING_WIFI   2
#define WAIT_CLOUD_CONFIG 3
#define SUCCESS           4
// errors
#define ERROR            -1
#define TIMEOUT          -2

class ProvState {
private:
  ProvState() : m_state(IDLE) {}
  volatile int m_state;

public:
  static ProvState& getInstance();
  int getState() const { return m_state; }
  void setState(int newState) { m_state = newState; } 
}; 