#include "ModuleSettingsManager.h"

ModuleSettingsManager::ModuleSettingsManager(WiFiManager& wifiManager)
  : m_wifiManager(wifiManager) {}
ModuleSettingsManager::~ModuleSettingsManager() {}

SetModuleSettingResult ModuleSettingsManager::handleSetModuleSetting(const String& id, const String& value) {
  SetModuleSettingResult result = { false, "" };

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
    Serial.printf("localIP:%s, gateway:%s, subnet:%s, dns1:%s, dns2:%s   \r\n", localIP.c_str(), gateway.c_str(), subnet.c_str(), dns1.c_str(), dns2.c_str());
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

  return result;
}