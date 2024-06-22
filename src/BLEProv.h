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
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <NimBLEUUID.h>

#include "ProvSettings.h"
#include "ProvDebug.h"
#include "CryptoMbedTLS.h" 
#include "ProvUtil.h"

class BLEProvClass : protected NimBLECharacteristicCallbacks, NimBLEServerCallbacks {
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
    void splitWrite(NimBLECharacteristic * pCharacteristic, const std::string& jsonString);

  protected:
    void handleKeyExchange(const std::string& public_key_pem, NimBLECharacteristic* pCharacteristic);
    void handleWiFiConfig(const std::string& wificonfig, NimBLECharacteristic* pCharacteristic);
    void handleCloudCredentialsConfig(const std::string& authconfig, NimBLECharacteristic* pCharacteristic);
    void handleWiFiList(NimBLECharacteristic* pCharacteristic);
    void handleProvInfo(NimBLECharacteristic* pCharacteristic);

    virtual void onWrite(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) override;
    virtual void onConnect(NimBLEServer* pServer) override;
    virtual void onConnect(BLEServer* pServer, ble_gap_conn_desc* desc) override;
    virtual void onDisconnect(NimBLEServer* pServer) override;
    virtual void onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc) override;

    bool m_begin; 
    String m_retailItemId;

    WiFiCredentialsCallbackHandler m_WiFiCredentialsCallbackHandler;
    CloudCredentialsCallbackHandler m_CloudCredentialsCallbackHandler;
    BleProvDoneCallbackHandler m_BleProvDoneCallbackHandler;

    NimBLEServer *m_pServer;
    NimBLEService *m_pService;
    NimBLEAdvertising *m_pAdvertising;

    NimBLECharacteristic *m_provWiFiConfig; 
    NimBLECharacteristic *m_provWiFiConfigNotify; 
    NimBLECharacteristic *m_provKeyExchange;
    NimBLECharacteristic *m_provKeyExchangeNotify;
    NimBLECharacteristic *m_provCloudCredentialConfig;  
    NimBLECharacteristic *m_provCloudCredentialConfigNotify;  
    NimBLECharacteristic *m_provWiFiList;  
    NimBLECharacteristic *m_provWiFiListNotify;  
    NimBLECharacteristic *m_provInfo;  
    NimBLECharacteristic *m_provInfoNotify;  
    
    CryptoMbedTLS m_crypto; 
    int m_expectedAuthConfigPayloadSize = -1;
    std::string m_receivedCloudCredentialsConfig;
    volatile bool m_provConfigDone = false;

    const std::string BLE_SERVICE_UUID                          = "0000ffff-0000-1000-8000-00805f9b34fb";

    const std::string BLE_WIFI_CONFIG_UUID                      = "00000001-0000-1000-8000-00805f9b34fb"; 
    const std::string BLE_KEY_EXCHANGE_UUID                     = "00000002-0000-1000-8000-00805f9b34fb"; 
    const std::string BLE_CLOUD_CREDENTIAL_CONFIG_UUID          = "00000003-0000-1000-8000-00805f9b34fb"; 
    const std::string BLE_WIFI_CONFIG_NOTIFY_UUID               = "00000004-0000-1000-8000-00805f9b34fb"; 
    const std::string BLE_WIFI_LIST_UUID                        = "00000005-0000-1000-8000-00805f9b34fb";    
    const std::string BLE_WIFI_LIST_NOTIFY_UUID                 = "00000006-0000-1000-8000-00805f9b34fb";    
    const std::string BLE_PROV_INFO_UUID                        = "00000007-0000-1000-8000-00805f9b34fb";    
    const std::string BLE_INFO_NOTIFY_UUID                      = "00000008-0000-1000-8000-00805f9b34fb";    
    const std::string BLE_CLOUD_CREDENTIAL_CONFIG_NOTIFY_UUID   = "00000009-0000-1000-8000-00805f9b34fb";
    const std::string BLE_KEY_EXCHANGE_NOTIFY_UUID              = "00000010-0000-1000-8000-00805f9b34fb";

    NimBLEUUID m_uuidService;   
    NimBLEUUID m_uuidWiFiConfig; 
    NimBLEUUID m_uuidWiFiConfigNotify;
    NimBLEUUID m_uuidKeyExchange;
    NimBLEUUID m_uuidKeyExchangeNotify;
    NimBLEUUID m_uuidCloudCredentialConfig;
    NimBLEUUID m_uuidCloudCredentialConfigNotify;
    NimBLEUUID m_uuidWiFiList;
    NimBLEUUID m_uuidWiFiListNotify;
    NimBLEUUID m_uuidProvInfo;
    NimBLEUUID m_uuidProvInfoNotify;
};
 