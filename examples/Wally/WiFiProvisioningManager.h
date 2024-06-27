#pragma once

#include <SinricProBusinessSdk.h>
#include "ConfigStore.h"
#include "WiFiManager.h"

class WiFiProvisioningManager {
public:
  WiFiProvisioningManager(ConfigStore& config);
  bool beginProvision(String productId);

private:
  ConfigStore& m_configStore;
  void handleButton(int state);
  void handleLedIndicator(int state);
};

WiFiProvisioningManager::WiFiProvisioningManager(ConfigStore& config)
  : m_configStore(config) {}

void WiFiProvisioningManager::handleLedIndicator(int state) {
  // TODO: Implement LED indicator handling
  // switch (state) {
  //   case IDLE:
  //   case WAIT_WIFI_CONFIG:
  //   case WAIT_CLOUD_CONFIG:
  //   case CONNECTING_WIFI:
  //   case SUCCESS:
  //   case TIMEOUT:
  //   case ERROR:
  //     break;
  // }
}

void WiFiProvisioningManager::handleButton(int state) {
  // TODO: Implement button handling
}

bool WiFiProvisioningManager::beginProvision(String productId) {
  WiFiProv prov(productId);

  prov.onWiFiCredentials([](const char* ssid, const char* password) -> bool {
    return WiFiManager::connectToWiFi(ssid, password);
  });

  prov.onCloudCredentials([this](const String& config) -> bool {
    JsonDocument jsonConfig;
    DeserializationError error = deserializeJson(jsonConfig, config);

    if (error) {
      Serial.printf("[WiFiProvisioningManager.beginProvision()]: deserializeJson() failed: %s\n", error.c_str());
      return false;
    } else {
      if (m_configStore.saveJsonConfig(jsonConfig)) {
        Serial.println(F("[WiFiProvisioningManager.beginProvision()]: Configuration updated!"));
        return true;
      } else {
        Serial.println(F("[WiFiProvisioningManager.beginProvision()]: Failed to save configuration!"));
        return false;
      }
    }
  });

  prov.loop([this](int state) {
    // Handle LED indicator or button press here if needed
    // Handle LED indicator or button press here if needed
    this->handleLedIndicator(state);
    this->handleButton(state);
  });

  return prov.beginProvision();
}