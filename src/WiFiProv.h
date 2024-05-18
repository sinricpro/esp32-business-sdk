/* 
  Copyright (c) 2019-2024 Sinric
*/

#pragma once 

#include <ArduinoJson.h>
#include "ProvSettings.h" 
#include "ProvUtil.h" 
#include "ProvDebug.h"
#include "ProvState.h"
#include "BLEProv.h"

class WiFiProv {
  public:
    using ProvDoneCallback = std::function<void(void)>;
    using WiFiCredentialsCallback = std::function<bool(const char* ssid, const char* password)>;
    using CloudCredentialsCallback = std::function<bool(const String &config)>;
    using LoopCallback = std::function<void(int state)>;
    
    WiFiProv(const String &retailItemId);
    ~WiFiProv();

    bool hasProvisioned();
    bool beginProvision();   
    void setConfigTimeout(int timeout);
    void setBlePrefix(String prefix = "");
    void onWiFiCredentials(WiFiCredentialsCallback cb);
    void onCloudCredentials(CloudCredentialsCallback cb);
    void loop(LoopCallback cb);

  private:
    void restart();
    
    // BLE
    bool onBleWiFiCredetials(String wifiConfig);
    bool startBLEConfig();
    void onBleProvDone();
    void onProvDone(ProvDoneCallback cb);
    bool onBleCloudCredetials(const String &config);  
 
    bool m_isConfigured;
    int m_timeout = DEFAULT_BLE_PROV_TIMEOUT;
    String m_ble_prefix;

    String m_retailItemId;
    ProvDoneCallback m_provDoneCallback; 
    WiFiCredentialsCallback m_wifiCredentialsCallback;
    CloudCredentialsCallback m_cloudCredentialsCallback;
    LoopCallback m_loopCallback;

    BLEProvClass BLEProv;
};
