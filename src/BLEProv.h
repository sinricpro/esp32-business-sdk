/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 * 
 *  @brief This class (BLEProvClass) provides functionalities for Bluetooth Low Energy (BLE) provisioning on an ESP32 device. 
 */

#pragma once 

#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUUID.h>
#include <BLE2902.h>
#include <BLECharacteristic.h>
#include <WiFi.h> 

#include "ProvSettings.h"
#include "ProvDebug.h"
#include "CryptoMbedTLS.h" 
#include "ProvUtil.h"

class BLEProvClass : public BLECharacteristicCallbacks, BLEServerCallbacks {
  public:
    using WiFiCredentialsCallbackHandler = std::function<bool(const String)>;
    using CloudCredentialsCallbackHandler = std::function<bool(const String)>;    
    using BleProvDoneCallbackHandler = std::function<void(void)>;
    
    BLEProvClass();
    virtual ~BLEProvClass();

    void begin(const String &deviceName, const String &retailItemId);
    void stop();
    void deinit();
    void onWiFiCredentials(WiFiCredentialsCallbackHandler cb);
    void onCloudCredentials(CloudCredentialsCallbackHandler cb);
    
    void onBleProvDone(BleProvDoneCallbackHandler cb);
    bool bleConfigDone();    
    String getBLEMac(); 
    void setProductId(const std::string &productId);

  private:
    void splitWrite(BLECharacteristic * pCharacteristic, const std::string& jsonString);

  protected:
    void handleKeyExchange(const std::string& public_key_pem, BLECharacteristic* pCharacteristic);
    void handleWiFiConfig(const std::string& wificonfig, BLECharacteristic* pCharacteristic);
    void handleCloudCredentialsConfig(const std::string& authconfig, BLECharacteristic* pCharacteristic);
    void handleWiFiList(BLECharacteristic* pCharacteristic);
    void handleProvInfo(BLECharacteristic* pCharacteristic);

    void onWrite(BLECharacteristic* pCharacteristic);
    void onConnect(BLEServer* pServer);
    void onDisconnect(BLEServer* pServer);
    void onMTUChange(uint16_t MTU);

    bool m_begin; 
    String m_retailItemId;

    WiFiCredentialsCallbackHandler m_WiFiCredentialsCallbackHandler;
    CloudCredentialsCallbackHandler m_CloudCredentialsCallbackHandler;
    BleProvDoneCallbackHandler m_BleProvDoneCallbackHandler;

    BLEServer *m_pServer = nullptr;
    BLEService *m_pService = nullptr;
    BLEAdvertising *m_pAdvertising = nullptr;

    BLECharacteristic *m_provWiFiConfig; 
    BLECharacteristic *m_provWiFiConfigNotify; 
    BLECharacteristic *m_provKeyExchange;
    BLECharacteristic *m_provKeyExchangeNotify;
    BLECharacteristic *m_provCloudCredentialConfig;  
    BLECharacteristic *m_provCloudCredentialConfigNotify;  
    BLECharacteristic *m_provWiFiList;  
    BLECharacteristic *m_provWiFiListNotify;  
    BLECharacteristic *m_provInfo;  
    BLECharacteristic *m_provInfoNotify;  
    
    CryptoMbedTLS m_crypto; 
    int m_expectedAuthConfigPayloadSize = -1;
    std::string m_receivedCloudCredentialsConfig;
    volatile bool m_provConfigDone = false;

    #define BLE_SERVICE_UUID                          "0000ffff-0000-1000-8000-00805f9b34fb"

    #define BLE_WIFI_CONFIG_UUID                      "00000001-0000-1000-8000-00805f9b34fb"
    #define BLE_WIFI_CONFIG_NOTIFY_UUID               "00000004-0000-1000-8000-00805f9b34fb" 

    #define BLE_KEY_EXCHANGE_UUID                     "00000002-0000-1000-8000-00805f9b34fb" 
    #define BLE_KEY_EXCHANGE_NOTIFY_UUID              "00000010-0000-1000-8000-00805f9b34fb"

    #define BLE_CLOUD_CREDENTIAL_CONFIG_UUID          "00000003-0000-1000-8000-00805f9b34fb" 
    #define BLE_CLOUD_CREDENTIAL_CONFIG_NOTIFY_UUID   "00000009-0000-1000-8000-00805f9b34fb"

    #define BLE_WIFI_LIST_UUID                        "00000005-0000-1000-8000-00805f9b34fb"    
    #define BLE_WIFI_LIST_NOTIFY_UUID                 "00000006-0000-1000-8000-00805f9b34fb"    

    #define BLE_PROV_INFO_UUID                        "00000007-0000-1000-8000-00805f9b34fb"    
    #define BLE_PROV_INFO_NOTIFY_UUID                 "00000008-0000-1000-8000-00805f9b34fb"
};
 