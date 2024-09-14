#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "FS.h"
#include "SPIFFS.h"
#include "Settings.h"

/**
 *  @brief Manages SinricPro using primary and secondary SSID configurations.
 */
class WiFiManager {
public:
  struct wifi_settings_t {
    char primarySSID[32];        ///< Primary SSID of the WiFi network.
    char primaryPassword[64];    ///< Primary password of the WiFi network.
    char secondarySSID[32];      ///< Secondary SSID of the WiFi network.
    char secondaryPassword[64];  ///< Secondary password of the WiFi network.
  };

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
  void printSettings() const;

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
  const wifi_settings_t& getWiFiSettings() const;

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
  const char* m_configFileName;            ///< File name to store WiFi settings.
  wifi_settings_t m_wifiSettings;          ///< WiFi settings.

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


};