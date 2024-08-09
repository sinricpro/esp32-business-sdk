
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "esp_system.h"
#include <esp_wifi.h>
#include <esp_heap_caps.h>

/**
 * @brief Class to handle health diagnostics
 */
class HealthManager {
public:
  /**
     * @brief Report the health diagnostic information.
     * 
     * @param healthReport A reference to a String to store the health report in JSON format.
     * @return True on success, otherwise false.
     */
  bool reportHealth(String& healthReport);

private:
  String getChipId();
  void addHeapInfo(JsonObject& doc);
  void addWiFiInfo(JsonObject& doc);
  void addSketchInfo(JsonObject& doc);
  void addResetCause(JsonObject& doc);
};
