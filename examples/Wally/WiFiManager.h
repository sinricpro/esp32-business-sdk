#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "FS.h"
#include "SPIFFS.h"
#include "Settings.h"

struct WifiSettings_t {
  char primarySSID[32];        ///< Primary SSID of the WiFi network.
  char primaryPassword[64];    ///< Primary password of the WiFi network.
  char secondarySSID[32];      ///< Secondary SSID of the WiFi network.
  char secondaryPassword[64];  ///< Secondary password of the WiFi network.
};

/**
 *  @brief Manages SinricPro using primary and secondary SSID configurations.
 */
class WiFiManager {
public:
  /**
   * @brief Construct a new WiFiManager object with default WiFi settings.
   * 
   * @param configFileName File name for storing WiFi settings.
   */
  WiFiManager(const char* configFileName = WIFI_CONFIG_FILE_NAME);

  /**
   * @brief Initializes the WiFi manager, loading settings from file or using defaults if loading fails.
   */
  bool loadConfig();

  /**
   * @brief Updates the primary WiFi settings.
   * 
   * @param newSSID New primary SSID.
   * @param newPassword New primary password.
   */
  bool updatePrimarySettings(const char* newSSID, const char* newPassword);

  /**
   * @brief Updates the secondary WiFi settings.
   * 
   * @param newSSID New secondary SSID.
   * @param newPassword New secondary password.
   */
  bool updateSecondarySettings(const char* newSSID, const char* newPassword);

  /**
   * @brief Prints the current WiFi settings to the Serial console.
   */
  void printSettings();

  /**
   * @brief Checks if the provided SSID and password are valid.
   * 
   * @param ssid SSID to check.
   * @param password Password to check.
   * @return true if both SSID and password are valid, false otherwise.
   */
  bool isValidSetting(const char* ssid, const char* password) const;

  /**
   * @brief Returns WiFi settings.
   */
  const WifiSettings_t& getWiFiSettings() const;

  /**
   * @brief Connect to primary or secondary WiFi.
   */
  bool connectToWiFi();

  /**
   * @brief Connect to specific WiFi.
   */
  bool connectToWiFi(const char* wifi_ssid, const char* wifi_password);

  bool setWiFiConfig(const String& localIP, const String& gateway, const String& subnet, const String& dns1, const String& dns2);

  /**
   * @brief Deletes all the stored WiFi settings.
   */
  void clear();

private:
  const char* m_configFileName;   ///< File name to store WiFi settings.
  WifiSettings_t m_wifiSettings;  ///< WiFi settings.

  /**
   * @brief Saves the current WiFi settings to a file.
   */
  bool saveToFile();

  /**
   * @brief Loads WiFi settings from a file.
   * 
   * @return true if loaded successfully, false otherwise.
   */
  bool loadFromFile();

  /**
   * @brief Validates the given SSID.
   * 
   * @param ssid SSID to validate.
   * @return true if the SSID is valid, false otherwise.
   */
  bool validateSSID(const char* ssid) const;

  /**
   * @brief Validates the given password.
   * 
   * @param password Password to validate.
   * @return true if the password is valid, false otherwise.
   */
  bool validatePassword(const char* password) const;

  /**
   * @brief Mask password.
   * 
   */
  void maskPassword(char* password, int showStart, int showEnd, char* maskedPassword);
};

WiFiManager::WiFiManager(const char* configFileName)
  : m_configFileName(configFileName) {
  memset(&m_wifiSettings, 0, sizeof(m_wifiSettings));
}

bool WiFiManager::loadConfig() {
  if (loadFromFile()) {
    printSettings();
    return true;
  } else {
    Serial.println("[WiFiManager::loadConfig()]: Load WiFi config failed!");
    return false;
  }
}

bool WiFiManager::updatePrimarySettings(const char* newSSID, const char* newPassword) {
  if (isValidSetting(newSSID, newPassword)) {
    strncpy(m_wifiSettings.primarySSID, newSSID, sizeof(m_wifiSettings.primarySSID));
    strncpy(m_wifiSettings.primaryPassword, newPassword, sizeof(m_wifiSettings.primaryPassword));
    return saveToFile();
  } else {
    Serial.println("[WiFiManager::updatePrimarySettings()]: Invalid Primary SSID or Password");
    return false;
  }
}

bool WiFiManager::updateSecondarySettings(const char* newSSID, const char* newPassword) {
  if (isValidSetting(newSSID, newPassword)) {
    strncpy(m_wifiSettings.secondarySSID, newSSID, sizeof(m_wifiSettings.secondarySSID));
    strncpy(m_wifiSettings.secondaryPassword, newPassword, sizeof(m_wifiSettings.secondaryPassword));
    return saveToFile();
  } else {
    Serial.println("[WiFiManager::updateSecondarySettings()]: Invalid Secondary SSID or Password");
    return false;
  }
}

void WiFiManager::printSettings() {
  if(strlen(m_wifiSettings.primarySSID) == 0) {
    Serial.printf("[WiFiManager::printSettings()]: Primary wifi setting is empty!\n");
  } else {
    Serial.printf("[WiFiManager::printSettings()]: Primary SSID: %s\n", m_wifiSettings.primarySSID);
    
    char maskedPassword[64];  
    maskPassword(m_wifiSettings.primaryPassword, 2, 3, maskedPassword);
    Serial.printf("[WiFiManager::printSettings()]: Primary Password: %s\n", maskedPassword);
  }
  
  if(strlen(m_wifiSettings.secondarySSID) != 0) {
    Serial.printf("[WiFiManager::printSettings()]: Secondary SSID: %s\n", m_wifiSettings.secondarySSID);

    char maskedPassword[64];  
    maskPassword(m_wifiSettings.secondaryPassword, 2, 3, maskedPassword);
    Serial.printf("[WiFiManager::printSettings()]: Secondary Password: %s\n", maskedPassword);
  }  
}

bool WiFiManager::saveToFile() {
  File file = SPIFFS.open(m_configFileName, FILE_WRITE);
  if (file) {
    /* below line will throw an exception if the SPIFFS is not setup */
    file.write(reinterpret_cast<const uint8_t*>(&m_wifiSettings), sizeof(m_wifiSettings));
    file.close();
    return true;
  } else {
    Serial.printf("[WiFiManager::saveToFile()]: Failed to save WiFi config to: %s\n", m_configFileName);
    return false;
  }
}

bool WiFiManager::loadFromFile() {
  File file = SPIFFS.open(m_configFileName, FILE_READ);
  if (file && file.size() == sizeof(m_wifiSettings)) {
    file.read(reinterpret_cast<uint8_t*>(&m_wifiSettings), sizeof(m_wifiSettings));
    file.close();
    return true;
  }
  return false;
}

void WiFiManager::clear() {
  memset(&m_wifiSettings, 0, sizeof(m_wifiSettings));
  if (SPIFFS.exists(m_configFileName)) {
    SPIFFS.remove(m_configFileName);
  }
  Serial.println("[WiFiManager::clear()]: All WiFi settings have been deleted.");
}

bool WiFiManager::isValidSetting(const char* ssid, const char* password) const {
  return validateSSID(ssid) && validatePassword(password);
}

bool WiFiManager::validateSSID(const char* ssid) const {
  return ssid && strlen(ssid) > 0 && strlen(ssid) < sizeof(WifiSettings_t::primarySSID);
}

bool WiFiManager::validatePassword(const char* password) const {
  return password && strlen(password) < sizeof(WifiSettings_t::primaryPassword);
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
      Serial.println("[WiFiManager::setWiFiConfig()]: STA Failed to configure");
      return false;
    }
  } else if (dns1 != "") {
    if (!WiFi.config(local_IP, gateway_IP, subnet_IP, dns1_IP)) {
      Serial.println("[WiFiManager::setWiFiConfig()]: STA Failed to configure");
      return false;
    }
  } else {
    if (!WiFi.config(local_IP, gateway_IP, subnet_IP)) {
      Serial.println("[WiFiManager::setWiFiConfig()]: STA Failed to configure");
      return false;
    }
  }

  return true;
}

const WifiSettings_t& WiFiManager::getWiFiSettings() const {
  return m_wifiSettings;
}

void WiFiManager::maskPassword(char* password, int showStart, int showEnd, char* maskedPassword) {
  int len = strlen(password);
  int maskLen = len - showStart - showEnd;
  
  if (maskLen < 0) {
    strcpy(maskedPassword, password);
    return;
  }
  
  strncpy(maskedPassword, password, showStart);
  for (int i = showStart; i < len - showEnd; i++) {
    maskedPassword[i] = '*';
  }
  strcpy(maskedPassword + len - showEnd, password + len - showEnd);
  maskedPassword[len] = '\0';
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
