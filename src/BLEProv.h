/* 
  Copyright (c) 2019-2024 Sinric
*/

#pragma once 

#include <NimBLEDevice.h>
#include <WiFi.h>
#include "CryptoMbedTLS.h" 
#include <ArduinoJson.h>
#include <NimBLEUUID.h>
#include "ProvDebug.h"
#include "ProvUtil.h" 
#include "ProvSettings.h"

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
    NimBLECharacteristic *m_provAuthConfig;  
    NimBLECharacteristic *m_provAuthConfigNotify;  
    NimBLECharacteristic *m_provWiFiList;  
    NimBLECharacteristic *m_provWiFiListNotify;  
    NimBLECharacteristic *m_provInfo;  
    NimBLECharacteristic *m_provInfoNotify;  
    
    CryptoMbedTLS m_crypto; 
    int m_expectedAuthConfigPayloadSize = -1;
    std::string m_receivedCloudCredentialsConfig;
    volatile bool m_provConfigDone = false;

    const std::string BLE_SERVICE_UUID            = "0000ffff-0000-1000-8000-00805f9b34fb";
    const std::string BLE_WIFI_CONFIG_UUID        = "00000001-0000-1000-8000-00805f9b34fb";
    const std::string BLE_KEY_EXCHANGE_UUID       = "00000002-0000-1000-8000-00805f9b34fb";
    const std::string BLE_AUTH_CONFIG_UUID        = "00000003-0000-1000-8000-00805f9b34fb";
    const std::string BLE_WIFI_CONFIG_NOTIFY_UUID = "00000004-0000-1000-8000-00805f9b34fb";    
    const std::string BLE_WIFI_LIST_UUID          = "00000005-0000-1000-8000-00805f9b34fb";    
    const std::string BLE_WIFI_LIST_NOTIFY_UUID   = "00000006-0000-1000-8000-00805f9b34fb";    
    const std::string BLE_INFO_UUID               = "00000007-0000-1000-8000-00805f9b34fb";    
    const std::string BLE_INFO_NOTIFY_UUID        = "00000008-0000-1000-8000-00805f9b34fb";    
    const std::string BLE_AUTH_CONFIG_NOTIFY_UUID = "00000009-0000-1000-8000-00805f9b34fb";

    NimBLEUUID m_uuidService;   
    NimBLEUUID m_uuidWiFiConfig; 
    NimBLEUUID m_uuidWiFiConfigNotify;
    NimBLEUUID m_uuidKeyExchange;
    NimBLEUUID m_uuidAuthConfig;
    NimBLEUUID m_uuidAuthConfigNotify;
    NimBLEUUID m_uuidWiFiList;
    NimBLEUUID m_uuidWiFiListNotify;
    NimBLEUUID m_uuidProvInfo;
    NimBLEUUID m_uuidProvInfoNotify;
};
 