/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 */

#include "WiFiManager.h"

WiFiManager::WiFiManager(const char* configFileName)
  : configFileName(configFileName) {
  memset(&wifiSettings, 0, sizeof(wifiSettings));
}

void WiFiManager::loadConfig() {
  if (!loadFromFile()) {
    saveDefaultSettings();
  }
  printSettings();
}

bool WiFiManager::updatePrimarySettings(const char* newSSID, const char* newPassword) {
  if (isValidSetting(newSSID, newPassword)) {
    strncpy(wifiSettings.primarySSID, newSSID, sizeof(wifiSettings.primarySSID));
    strncpy(wifiSettings.primaryPassword, newPassword, sizeof(wifiSettings.primaryPassword));
    return saveToFile();
  } else {
    Serial.println("Invalid Primary SSID or Password");
    return false;
  }
}

bool WiFiManager::updateSecondarySettings(const char* newSSID, const char* newPassword) {
  if (isValidSetting(newSSID, newPassword)) {
    strncpy(wifiSettings.secondarySSID, newSSID, sizeof(wifiSettings.secondarySSID));
    strncpy(wifiSettings.secondaryPassword, newPassword, sizeof(wifiSettings.secondaryPassword));
    return saveToFile();
  } else {
    Serial.println("Invalid Secondary SSID or Password");
    return false;
  }
}

void WiFiManager::printSettings() const {
  Serial.printf("Primary SSID: %s\n", wifiSettings.primarySSID);
  Serial.printf("Primary Password: %s\n", wifiSettings.primaryPassword);
  Serial.printf("Secondary SSID: %s\n", wifiSettings.secondarySSID);
  Serial.printf("Secondary Password: %s\n", wifiSettings.secondaryPassword);
}

bool WiFiManager::saveToFile() {
  File file = LittleFS.open(configFileName, FILE_WRITE);
  if (file) {
    file.write(reinterpret_cast<const uint8_t*>(&wifiSettings), sizeof(wifiSettings));
    file.close();
    return true;
  } else {
    Serial.printf("Failed to save WiFi\n");
    return false;
  }
}

bool WiFiManager::loadFromFile() {
  File file = LittleFS.open(configFileName, FILE_READ);
  if (file && file.size() == sizeof(wifiSettings)) {
    file.read(reinterpret_cast<uint8_t*>(&wifiSettings), sizeof(wifiSettings));
    file.close();
    return true;
  }
  return false;
}

void WiFiManager::saveDefaultSettings() {
  Serial.println("Saving default WiFi login!");

  strncpy(wifiSettings.primarySSID, defaultPrimarySSID, sizeof(wifiSettings.primarySSID));
  strncpy(wifiSettings.primaryPassword, defaultPrimaryPassword, sizeof(wifiSettings.primaryPassword));
  strncpy(wifiSettings.secondarySSID, defaultSecondarySSID, sizeof(wifiSettings.secondarySSID));
  strncpy(wifiSettings.secondaryPassword, defaultSecondaryPassword, sizeof(wifiSettings.secondaryPassword));

  saveToFile();
}

void WiFiManager::deleteAllSettings() {
  memset(&wifiSettings, 0, sizeof(wifiSettings));
  if (LittleFS.exists(configFileName)) {
    LittleFS.remove(configFileName);
  }
  Serial.println("All WiFi settings have been deleted.");
}

bool WiFiManager::isValidSetting(const char* ssid, const char* password) const {
  return validateSSID(ssid) && validatePassword(password);
}

bool WiFiManager::validateSSID(const char* ssid) const {
  return ssid && strlen(ssid) > 0 && strlen(ssid) < sizeof(wifi_settings_t::primarySSID);
}

bool WiFiManager::validatePassword(const char* password) const {
  return password && strlen(password) < sizeof(wifi_settings_t::primaryPassword);
}

bool WiFiManager::setWiFiConfig(const String& localIP, const String& gateway, const String& subnet, const String& dns1, const String& dns2) {
  IPAddress local_IP, gateway_IP, subnet_IP, dns1_IP, dns2_IP;
  local_IP.fromString(localIP);
  gateway_IP.fromString(gateway);
  subnet_IP.fromString(subnet);

  if (dns1 != "") {
    dns1_IP.fromString(dns1);
  }
  if (dns2 != "") {
    dns2_IP.fromString(dns2);
  }

  // Configure static IP address
  if (dns1 != "" && dns2 != "") {
    if (!WiFi.config(local_IP, gateway_IP, subnet_IP, dns1_IP, dns2_IP)) {
      Serial.println("STA Failed to configure");
      return false;
    }
  } else if (dns1 != "") {
    if (!WiFi.config(local_IP, gateway_IP, subnet_IP, dns1_IP)) {
      Serial.println("STA Failed to configure");
      return false;
    }
  } else {
    if (!WiFi.config(local_IP, gateway_IP, subnet_IP)) {
      Serial.println("STA Failed to configure");
      return false;
    }
  }

  return true;
}

const WiFiManager::wifi_settings_t& WiFiManager::getWiFiSettings() const {
  return wifiSettings;
}

bool WiFiManager::connectToWiFi() {
  auto& settings = getWiFiSettings();
  bool connected = false;

  if (isValidSetting(settings.primarySSID, settings.primaryPassword)) {
    connected = connectToWiFi(settings.primarySSID, settings.primaryPassword);
  }

  if (!connected && isValidSetting(settings.secondarySSID, settings.secondaryPassword)) {
    connected = connectToWiFi(settings.secondarySSID, settings.secondaryPassword);
  }

  if (connected) {
    Serial.println("Connected to WiFi!");
  } else {
    Serial.println("Failed to connect to WiFi!");
  }

  return connected;
}

bool WiFiManager::connectToWiFi(const char* wifi_ssid, const char* wifi_password) {
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0))
  WiFi.setMinSecurity(WIFI_AUTH_WEP);  // https://github.com/espressif/arduino-esp32/blob/master/docs/source/troubleshooting.rst
#endif

  WiFi.disconnect();
  delay(10);

  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.persistent(true);
  WiFi.setSleep(false);
  WiFi.begin(wifi_ssid, wifi_password);

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 30) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WiFiManager.connectToWiFi()]: WiFi connected.");
    Serial.printf("IP: %s\r\n", WiFi.localIP().toString().c_str());
    WiFi.setAutoReconnect(true);
    return true;
  } else {
    Serial.printf("[WiFiManager.connectToWiFi()]: WiFi connection failed!\r\n");
    return false;
  }
}