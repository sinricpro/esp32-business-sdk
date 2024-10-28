#pragma once

#include "inc/WiFiManager.h"

// Define constants for setting types
#define SET_WIFI_PRIMARY "pro.sinric::set.wifi.primary"
#define SET_WIFI_SECONDARY "pro.sinric::set.wifi.secondary"
#define SET_FIXED_IP_ADDRESS "pro.sinric::set.fixed.ip.address"

/**
 * @struct SetModuleSettingResult_t
 * @brief Represents the result of a module setting operation.
 */
struct SetModuleSettingResult_t {
  bool success;    ///< Indicates whether the operation was successful
  String message;  ///< Contains a message describing the result or any error
};

/**
 * @class ModuleSettingsManager
 * @brief Manages module settings for the ESP32 device.
 * 
 * This class provides functionality to handle various module settings,
 * particularly related to WiFi configuration.
 */
class ModuleSettingsManager {
public:
  /**
     * @brief Constructor for ModuleSettingsManager.
     * @param wifiManager Reference to the WiFiManager object.
     */
  ModuleSettingsManager(WiFiManager& wifiManager);

  /**
     * @brief Destructor for ModuleSettingsManager.
     */
  ~ModuleSettingsManager();

  /**
     * @brief Handles setting a module configuration.
     * @param id The identifier for the setting to be changed.
     * @param value The new value for the setting.
     * @return SetModuleSettingResult_t Struct containing the result of the operation.
     */
  SetModuleSettingResult_t handleSetModuleSetting(const String& id, const String& value);

private:
  WiFiManager m_wifiManager;  ///< WiFiManager instance to handle WiFi-related operations
};

ModuleSettingsManager::ModuleSettingsManager(WiFiManager& wifiManager)
  : m_wifiManager(wifiManager) {}
ModuleSettingsManager::~ModuleSettingsManager() {}

SetModuleSettingResult_t ModuleSettingsManager::handleSetModuleSetting(const String& id, const String& value) {
  SetModuleSettingResult_t result = { false, "" };

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, value);
  if (error) {
    result.message = "handleSetModuleSetting::deserializeJson() failed: " + String(error.c_str());
    Serial.println(result.message);
    return result;
  }

  const char* password = doc["password"].as<const char*>();
  const char* ssid = doc["ssid"].as<const char*>();
  bool connectNow = doc["connectNow"] | false;

  if (id == SET_WIFI_PRIMARY) {  // Set primary WiFi
    if (m_wifiManager.updatePrimarySettings(ssid, password)) {
      result.message = "Primary WiFi settings updated successfully.";
      result.success = true;
    } else {
      result.message = "Primary WiFi update failed!";
    }
  } else if (id == SET_WIFI_SECONDARY) {  // Set secondary WiFi
    if (m_wifiManager.updateSecondarySettings(ssid, password)) {
      result.message = "Secondary WiFi settings updated successfully.";
      result.success = true;
    } else {
      result.message = "Secondary WiFi update failed!";
    }
  } else if (id == SET_FIXED_IP_ADDRESS) {
    String localIP = doc["localIP"];
    String gateway = doc["gateway"];
    String subnet = doc["subnet"];
    String dns1 = doc["dns1"] | "";
    String dns2 = doc["dns2"] | "";

    // Change your WiFi config here.
    Serial.printf("[ModuleSettingsManager.handleSetModuleSetting()]: localIP:%s, gateway:%s, subnet:%s, dns1:%s, dns2:%s   \r\n", localIP.c_str(), gateway.c_str(), subnet.c_str(), dns1.c_str(), dns2.c_str());
    result.success = m_wifiManager.setWiFiConfig(localIP, gateway, subnet, dns1, dns2);
  } else {
    result.message = "Invalid setting ID.";
    return result;
  }

  if (result.success && connectNow) {
    if (m_wifiManager.connectToWiFi(ssid, password)) {
      result.message += " Connected to WiFi successfully.";
    } else {
      result.message += " Failed to connect to WiFi.";
      result.success = false;
    }
  }

  Serial.printf("[ModuleSettingsManager.handleSetModuleSetting()]: message:%s \r\n", result.message.c_str());
  return result;
}