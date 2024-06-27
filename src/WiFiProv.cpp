/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 */

#include "WiFiProv.h"


/**
* @brief constructor.
* @param retailItemId
*      retailItem id.
*/ 
WiFiProv::WiFiProv(const String &retailItemId) : m_retailItemId(retailItemId) {
  m_isConfigured = false;
}

/**
* @brief BLE provisioning timeout. Default: 45 mins
*/ 
void WiFiProv::setConfigTimeout(int timeout) {
  m_timeout = timeout;
}

void WiFiProv::setBlePrefix(String prefix) {
  m_ble_prefix = prefix;
}


 /**
 * @brief Check whether device has been provisioned already.
 * 
 * @retval true
 *      Provisioned
 * @retval false
 *      Not provisioned
 */
bool WiFiProv::hasProvisioned() { 
  return m_isConfigured;
}

/**
 * @brief Start device provisioning.
 * @retval true
 *      Provisioning success
 * @retval false
 *      Provisioning failed
 */
bool WiFiProv::beginProvision() {
  if(!m_wifiCredentialsCallback) {
    DEBUG_PROV(PSTR("[WiFiProv.beginProvision()]: WiFi credential callback not set! Cannot continue!!"));
    return false;
  }

  if(!m_cloudCredentialsCallback) {
    DEBUG_PROV(PSTR("[WiFiProv.beginProvision()]: Cloud credential callback not set! Cannot continue!!"));
    return false;
  }  

  if (!m_isConfigured) {
    m_isConfigured = startBLEConfig(); 
    
    if(!m_isConfigured) {
      DEBUG_PROV(PSTR("[WiFiProv.beginProvision()]: Provisioing failed!...\r\n"));
    }
  } else {
    DEBUG_PROV(PSTR("[WiFiProv.beginProvision()]: Already provisioned!"));
  }

  return m_isConfigured;
}
 
WiFiProv::~WiFiProv(){}


/**
* @brief Gets called when appkey/appsecret and device ids are recevied by the app.
* @param config
*      appkey/appsecret and device ids in json format
* @return
*      ok
*/ 
bool WiFiProv::onBleCloudCredetials(const String &config) {
  DEBUG_PROV(PSTR("[WiFiProv.onAuthCredetials()]: JSON: %s\r\n"), config.c_str());  
  return m_cloudCredentialsCallback(config);
}

/**
* @brief Set the callback to receive cloud credentials.
* @param cb callback
*/ 
void WiFiProv::onCloudCredentials(CloudCredentialsCallback cb) {
  m_cloudCredentialsCallback = cb;
}

/**
* @brief Gets called when wifi credentials are recevied via BLE from the app.
* @param wifiConfig
*      WiFi SSID/Password in json format
* @return
*      ok
*/ 
bool WiFiProv::onBleWiFiCredetials(String wifiConfig) {
  bool success = false;
 
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, wifiConfig);
  if (error) {
      DEBUG_PROV(PSTR("[WiFiProv.onBleWiFiCredetials()]: deserializeJson() failed: %s"), error.c_str());
      return false;
  }

  const char * ssid = doc["ssid"];
  const char * pass = doc["pass"];

  ProvState& provState = ProvState::getInstance();
  provState.setState(CONNECTING_WIFI);

  success = m_wifiCredentialsCallback(ssid, pass);

  if (success) {
    provState.setState(WAIT_CLOUD_CONFIG);
  } else {
    provState.setState(ERROR);
  }
    
 return success;
}

/**
* @brief Get called when BLE provisioing is done.
*/ 
void WiFiProv::onBleProvDone() {
  if(m_provDoneCallback) {
      m_provDoneCallback();
  } 
}

/**
* @brief Set the callback to invoke when provisioning done.
* @param cb callback
*/ 
void WiFiProv::onProvDone(ProvDoneCallback cb) {
  m_provDoneCallback = cb;
}

void WiFiProv::loop(LoopCallback cb) {
  m_loopCallback = cb;
}


/**
* @brief Set the callback to invoke receive wifi credentials.
* @param cb callback
*/ 
void WiFiProv::onWiFiCredentials(WiFiCredentialsCallback cb) {
  m_wifiCredentialsCallback = cb;
}

/**
 * @brief Start bluetooth provisioning and wait-until timeout.
 * @return ok
 */ 
bool WiFiProv::startBLEConfig() {
  DEBUG_PROV(PSTR("[WiFiProv.startBLEConfig()]: Setup BLE provisioning.. \r\n"));

  // Setup callbacks from BLE  
  BLEProv.onWiFiCredentials(std::bind(&WiFiProv::onBleWiFiCredetials, this, std::placeholders::_1));
  BLEProv.onCloudCredentials(std::bind(&WiFiProv::onBleCloudCredetials, this, std::placeholders::_1));
  BLEProv.onBleProvDone(std::bind(&WiFiProv::onBleProvDone, this));
  
  String bleHostName = String();
  bleHostName.concat(BLE_HOST_PREFIX); // DO NOT CHANGE
  bleHostName.concat(m_ble_prefix);
  bleHostName.concat(String(ProvUtil::getChipId32(), HEX));
  
  BLEProv.begin(bleHostName, m_retailItemId);
  DEBUG_PROV(PSTR("[WiFiProv.startBLEConfig()]: Waiting for credentials. BLE Host Name: [%s]\r\n"), bleHostName.c_str()); 

  ProvState& provState = ProvState::getInstance();
  provState.setState(WAIT_WIFI_CONFIG);

  unsigned long start = millis();
  bool didTimeout = false;
  
  while (1) {
    delay(50); // TODO Need?
    
    if(m_loopCallback) m_loopCallback(provState.getState());

    if (BLEProv.bleConfigDone()) {
      provState.setState(SUCCESS);
      DEBUG_PROV(PSTR("[WiFiProv.startBLEConfig()]: BLE setup completed!\r\n")); 
      BLEProv.stop();
      BLEProv.deinit();
      break;
    }
    
    didTimeout = (millis() > start + m_timeout);

    if (didTimeout) {
        BLEProv.stop(); 
        BLEProv.deinit();
        delay(1000);         
        DEBUG_PROV(PSTR("[WiFiProv.startBLEConfig()]: BLE config timed out!\r\n"));  
        provState.setState(TIMEOUT);
        break;
    } 
  }    

  return (didTimeout == true ? false : true);
}
 

/**
 * @brief Restart the ESP module
 */
void WiFiProv::restart() {
  DEBUG_PROV(PSTR("[WiFiProv.restart()]: Restarting ESP ..\r\n"));
  ESP.restart();
  while(1){}  
}
