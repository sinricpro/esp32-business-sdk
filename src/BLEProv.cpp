/* 
  Copyright (c) 2019-2024 Sinric
*/

#include "BLEProv.h"

BLEProvClass::BLEProvClass() 
: m_begin(false)
, m_WiFiCredentialsCallbackHandler(nullptr),
  m_CloudCredentialsCallbackHandler(nullptr),
  m_receivedCloudCredentialsConfig(""),
  m_uuidService(BLE_SERVICE_UUID),
  m_uuidWiFiConfig(BLE_WIFI_CONFIG_UUID),
  m_uuidWiFiConfigNotify(BLE_WIFI_CONFIG_NOTIFY_UUID),
  m_uuidKeyExchange(BLE_KEY_EXCHANGE_UUID),
  m_uuidAuthConfig(BLE_AUTH_CONFIG_UUID),  
  m_uuidAuthConfigNotify(BLE_AUTH_CONFIG_NOTIFY_UUID),
  m_uuidWiFiList(BLE_WIFI_LIST_UUID),
  m_uuidWiFiListNotify(BLE_WIFI_LIST_NOTIFY_UUID),
  m_uuidProvInfo(BLE_INFO_UUID),
  m_uuidProvInfoNotify(BLE_INFO_NOTIFY_UUID){}

/**
* @brief Setup BLE provisioning endpoints 
*/
void BLEProvClass::begin(const String &deviceName, const String &retailItemId) {
  if (m_begin) stop();

  DEBUG_PROV(PSTR("[BLEProvClass.begin] Setup BLE endpoints ..\r\n"));
  
  NimBLEDevice::init(deviceName.c_str());
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); 
  NimBLEDevice::setMTU(512);
   
  m_pServer = NimBLEDevice::createServer();
  m_pServer->setCallbacks(this);
  m_pServer->advertiseOnDisconnect(false);

  m_pService = m_pServer->createService(m_uuidService);

  m_provWiFiConfig = m_pService->createCharacteristic(m_uuidWiFiConfig, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE); 
  m_provWiFiConfig->setValue("wifi_config");
  m_provWiFiConfig->setCallbacks(this);

  m_provKeyExchange = m_pService->createCharacteristic(m_uuidKeyExchange, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE); 
  m_provKeyExchange->setValue("key_exchange");
  m_provKeyExchange->setCallbacks(this); 

  m_provAuthConfig = m_pService->createCharacteristic(m_uuidAuthConfig, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE); 
  m_provAuthConfig->setValue("auth_config");
  m_provAuthConfig->setCallbacks(this); 

  m_provWiFiConfigNotify = m_pService->createCharacteristic(m_uuidWiFiConfigNotify, NIMBLE_PROPERTY::NOTIFY); 
  m_provWiFiConfigNotify->setValue("wifi_config_notify");
  m_provWiFiConfigNotify->setCallbacks(this);

  m_provWiFiList = m_pService->createCharacteristic(m_uuidWiFiList, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE); 
  m_provWiFiList->setValue("wifi_list");
  m_provWiFiList->setCallbacks(this); 

  m_provWiFiListNotify = m_pService->createCharacteristic(m_uuidWiFiListNotify, NIMBLE_PROPERTY::NOTIFY); 
  m_provWiFiListNotify->setValue("wifi_list_notify");
  m_provWiFiListNotify->setCallbacks(this);

  m_provInfo = m_pService->createCharacteristic(m_uuidProvInfo, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE); 
  m_provInfo->setValue("prov_info");
  m_provInfo->setCallbacks(this); 

  m_provInfoNotify = m_pService->createCharacteristic(m_uuidProvInfoNotify, NIMBLE_PROPERTY::NOTIFY); 
  m_provInfoNotify->setValue("prov_info_notify");
  m_provInfoNotify->setCallbacks(this);

  m_provAuthConfigNotify = m_pService->createCharacteristic(m_uuidAuthConfigNotify, NIMBLE_PROPERTY::NOTIFY); 
  m_provAuthConfigNotify->setValue("auth_config_info_notify");
  m_provAuthConfigNotify->setCallbacks(this);
 
  m_pService->start();
  
  m_pAdvertising = NimBLEDevice::getAdvertising();
  m_pAdvertising->setScanResponse(true); // must be true or else BLE name gets truncated
  m_pAdvertising->addServiceUUID(m_uuidService);
  m_pAdvertising->start(); 

  DEBUG_PROV(PSTR("[BLEProvClass.begin] done!\r\n"));
  m_retailItemId = retailItemId;  
  m_begin = true;
}

/**
* @brief Generate a session encryption key using client's public key.
*/
void BLEProvClass::handleKeyExchange(const std::string& publicKey, NimBLECharacteristic* pCharacteristic) {
  DEBUG_PROV(PSTR("[BLEProvClass.handleKeyExchange()] Start!\r\n"));

  struct KeyExchangeData {
    BLEProvClass* provClass;
    NimBLECharacteristic* characteristic;
    const char* publicKey;
  };

  KeyExchangeData* data = (KeyExchangeData*)malloc(sizeof(KeyExchangeData));
  if (data == nullptr) {
    DEBUG_PROV(PSTR("[BLEProvClass.handleKeyExchange()] ProvData allocation failed!\r\n"));
    return;
  }

  data->provClass = this;
  data->characteristic = pCharacteristic;
  data->publicKey = publicKey.c_str();

  void (*onCharacteristicWriteTask)(void*) = [](void* param) {
    KeyExchangeData* data = static_cast<KeyExchangeData*>(param);
    std::string sessionKey;
    std::string publicKey(data->publicKey);
  
    if(data->provClass->m_crypto.initMbedTLS()) {
     data->provClass->m_crypto.getSharedSecret(publicKey, sessionKey);      
    }    

    data->provClass->m_crypto.deinitMbedTLS();

    DEBUG_PROV(PSTR("[BLEProvClass.handleKeyExchange()] Encrypted session key is: %s\r\n"), sessionKey.c_str());      

    data->characteristic->setValue(sessionKey);
    data->characteristic->notify(true);
    
    vTaskDelete(NULL);
  };

  xTaskCreatePinnedToCore(onCharacteristicWriteTask, "BLEProvCharacteristicTask", 12288, data, 0, NULL, 0); // MbedTLS needs a large stack size
}

 
/**
* @brief Called when mobile sends Sinric Pro credentials (appkey, secret, devceids). 
* Mobile sends authentication config string in chucks due to BLE limitations
*/
void BLEProvClass::handleCloudCredentialsConfig(const std::string& cloudCredentialsConfigChuck, NimBLECharacteristic* pCharacteristic) {
  if(m_expectedAuthConfigPayloadSize == -1) {
      m_expectedAuthConfigPayloadSize = std::atoi(cloudCredentialsConfigChuck.c_str());
      DEBUG_PROV(PSTR("[BLEProvClass.handleCloudCredentialsConfig()] Expected config payload size: %d\r\n"), m_expectedAuthConfigPayloadSize);
  } else {
    // Append data chucks
    m_receivedCloudCredentialsConfig.append(cloudCredentialsConfigChuck);
    DEBUG_PROV(PSTR("[BLEProvClass.handleCloudCredentialsConfig()] %d/%d\r\n"), m_receivedCloudCredentialsConfig.size(), m_expectedAuthConfigPayloadSize);
    
    if(m_receivedCloudCredentialsConfig.size() == m_expectedAuthConfigPayloadSize) {
      m_expectedAuthConfigPayloadSize = -1;
      DEBUG_PROV(PSTR("[BLEProvClass.handleCloudCredentialsConfig()] Auth config payload receive completed\r\n")); 
      
      if (m_CloudCredentialsCallbackHandler) {
         std::vector<uint8_t> decodedConfig = m_crypto.base64Decode(m_receivedCloudCredentialsConfig);
         m_crypto.aesCTRXdecrypt(m_crypto.key, m_crypto.iv, decodedConfig);
         std::string authConfig(decodedConfig.begin(), decodedConfig.end());    
    
         DEBUG_PROV(PSTR("[BLEProvClass.handleCloudCredentialsConfig()] Decrypted config: %s\r\n"), authConfig.c_str());  
    
         // Calling callback to connect to WiFi      
         bool success = m_CloudCredentialsCallbackHandler(String(authConfig.c_str())); 
         std::string jsonString;
         
         StaticJsonDocument<24> doc;
         doc["success"] = success ? true : false;
         serializeJsonPretty(doc, jsonString); 
         DEBUG_PROV(PSTR("[BLEProvClass.handleCloudCredentialsConfig()] Response: %s\r\n"), jsonString.c_str());    
    
         splitWrite(m_provAuthConfigNotify, jsonString);

         DEBUG_PROV(PSTR("[BLEProvClass.handleCloudCredentialsConfig()] Notified!\r\n"));    

         // Wait until client gets the response before we wrap up.
         ProvUtil::wait(2000);
      
         m_provConfigDone = true;
          
         if(success && m_BleProvDoneCallbackHandler) {
            m_BleProvDoneCallbackHandler();
         }
        } else {
          DEBUG_PROV(PSTR("[BLEProvClass.handleCloudCredentialsConfig()] Auth callback not defined!\r\n"));  
          
          std::string jsonString;
          StaticJsonDocument<200> doc;
          doc[F("success")] = false;
          doc[F("message")] = F("Failed set authentication (nocallback)..");
          serializeJsonPretty(doc, jsonString);
          pCharacteristic->setValue(jsonString);
          pCharacteristic->notify(true);
        }    
    }          
  }     
}  

/**
* @brief Called when mobile sends WiFi credentials.
*/
void BLEProvClass::handleWiFiConfig(const std::string& wificonfig, NimBLECharacteristic* pCharacteristic) {
  DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()] Start!\r\n"));  

  if (m_WiFiCredentialsCallbackHandler) {
     std::vector<uint8_t> decoded = m_crypto.base64Decode(wificonfig);
     m_crypto.aesCTRXdecrypt(m_crypto.key, m_crypto.iv, decoded);
     std::string wiFi_config(decoded.begin(), decoded.end());

     DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()] Wi-Fi config: %s\r\n"), wiFi_config.c_str());  
     
     bool success = m_WiFiCredentialsCallbackHandler(String(wiFi_config.c_str())); 

     std::string jsonString = "";
     
     if(success) {
        StaticJsonDocument<200> doc;
        doc[F("success")] = true;
        doc[F("message")] = F("Success!");
        doc[F("bssid")] = WiFi.macAddress();
        doc[F("ip")] = WiFi.localIP().toString();
        serializeJsonPretty(doc, jsonString); 
     } else {
        StaticJsonDocument<200> doc;
        doc[F("success")] = false;
        doc[F("message")] = F("Failed to connect to WiFi..!");
        serializeJsonPretty(doc, jsonString);
     }

     DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()] WiFi Config response size: %u\r\n"), jsonString.length());    
     DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()] WiFi Config response: %s\r\n"), jsonString.c_str());    
      
     splitWrite(m_provWiFiConfigNotify, jsonString);

     DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()] Done!\r\n"));          
    } else {
      DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()] m_WiFiCredentialsCallbackHandler not set!\r\n"));    
      
      std::string jsonString;
      StaticJsonDocument<200> doc;
      doc[F("success")] = false;
      doc[F("message")] = F("Wifi Credentials Callback not set!..");
      serializeJsonPretty(doc, jsonString);
      m_provWiFiConfigNotify->setValue(jsonString);
      m_provWiFiConfigNotify->notify();
   }

   DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()] End!\r\n"));   
}

/**
* @brief Called when mobile wants a list of WiFis ESP can connect to.
*/
void BLEProvClass::handleWiFiList(NimBLECharacteristic* pCharacteristic) {
  DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiList()] Start!\r\n"));  

  int total = WiFi.scanNetworks();

  String jsonString = "[";
  for (int i = 0; i < total; ++i) {
      if(i != 0) jsonString += ",";
      jsonString += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
      jsonString += "\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  jsonString += "]";

  DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiList()] WiFi list: %s\r\n"), jsonString.c_str());

  // Free memory
  WiFi.scanDelete();
   
  splitWrite(m_provWiFiListNotify, std::string(jsonString.c_str()));      
      
  DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiList()] End!\r\n"));    
}

/**
* @brief Split the data into chunks and write. App will reassemble the complete data from these fragments.
*/
void BLEProvClass::splitWrite(NimBLECharacteristic * pCharacteristic, const std::string& data) {
  // Write length
  pCharacteristic->setValue(ProvUtil::to_string(data.length()));
  pCharacteristic->notify(true);
  delay(500);

  // Write data
  int offset          = 0;
  int remainingLength = data.length();
  const uint8_t* str  = reinterpret_cast<const uint8_t*>(data.c_str());

  while (remainingLength > 0) {
    int bytesToSend = min(BLE_FRAGMENT_SIZE, remainingLength); // send in chunks bytes until all the bytes are sent
    DEBUG_PROV(PSTR("[BLEProvClass.splitWrite()] Sending %u bytes!\r\n"), bytesToSend);    
    pCharacteristic->setValue((str + offset), bytesToSend);
    pCharacteristic->notify();
    delay(10);
    remainingLength -= bytesToSend;
    offset += bytesToSend;
  }
}

/**
* @brief Called when mobile wants a information about this device.
*/
void BLEProvClass::handleProvInfo(NimBLECharacteristic* pCharacteristic) {
  DEBUG_PROV(PSTR("[BLEProvClass.handleProvInfo()] Start!\r\n"));  

  std::string jsonString;
  StaticJsonDocument<200> doc;
  doc[F("retailItemId")] = m_retailItemId;
  doc[F("version")] = BLE_PROV_VERSION;
       
  serializeJsonPretty(doc, jsonString);

  DEBUG_PROV(PSTR("[BLEProvClass.handleProvInfo()] Write: %s\r\n"), jsonString.c_str());  
  
  splitWrite(m_provInfoNotify, jsonString); 

  DEBUG_PROV(PSTR("[BLEProvClass.handleProvInfo()] End!\r\n"));    
}

void BLEProvClass::onWrite(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) {
  DEBUG_PROV(PSTR("[BLEProvClass.onWrite()] UUID: %s, Got: %s\r\n"), pCharacteristic->getUUID().toString().c_str(), pCharacteristic->getValue().c_str());
  if (pCharacteristic == m_provKeyExchange && m_provKeyExchange->getDataLength()) { 
     handleKeyExchange(m_provKeyExchange->getValue(), pCharacteristic); 
  }        
  else if (pCharacteristic == m_provWiFiConfig && m_provWiFiConfig->getDataLength()) { 
    handleWiFiConfig(m_provWiFiConfig->getValue(), pCharacteristic);
  }
  else if (pCharacteristic == m_provAuthConfig && m_provAuthConfig->getDataLength()) { 
    handleCloudCredentialsConfig(m_provAuthConfig->getValue(), pCharacteristic);
  }
  else if (pCharacteristic == m_provWiFiList) { 
    handleWiFiList(pCharacteristic);
  }
  else if (pCharacteristic == m_provInfo) { 
    handleProvInfo(pCharacteristic);
  } else {
    DEBUG_PROV(PSTR("[BLEProvClass.onWrite()] Characteristic not found!"));
  }     
}

/**
* @brief Called when client connect.
*/
void BLEProvClass::onConnect(NimBLEServer* pServer) {
  DEBUG_PROV(PSTR("[BLEProvClass.onConnect()]: Client connected\r\n"));
}

/**
* @brief Show connected client MTU.
*/
void BLEProvClass::onConnect(BLEServer* pServer, ble_gap_conn_desc* desc) {
  DEBUG_PROV(PSTR("[BLEProvClass.onConnect()]: MTU of client: %d\r\n"), pServer->getPeerMTU(desc->conn_handle));
}

/**
* @brief Called when client disconnect..
*/
void BLEProvClass::onDisconnect(NimBLEServer* pServer) {
  DEBUG_PROV(PSTR("[BLEProvClass.onDisconnect()]: Client disconnected\r\n"));

  if (m_begin) { 
    DEBUG_PROV(PSTR("[BLEProvClass.onDisconnect()]: Start advertising\r\n"));
    m_pAdvertising->start();
  } 
}

/**
* @brief Called when client request for different MTU..
*/
void BLEProvClass::onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc) {
  DEBUG_PROV(PSTR("[BLEProvClass.onMTUChange()]: MTU updated: %u for connection ID: %u\r\n"), MTU, desc->conn_handle);
};

/**
* @brief Stop BLE provisioning..
*/
void BLEProvClass::stop() {
  // do not deinit here. still sending data to mobile.
  if (m_begin) {
    m_pAdvertising->stop();
    m_begin = false;
  }
}

/**
* @brief Deinit BLE ..
*/
void BLEProvClass::deinit() {
  NimBLEDevice::deinit();
}

/**
* @brief Get called when receive WiFi credentials
*/
void BLEProvClass::onWiFiCredentials(WiFiCredentialsCallbackHandler cb) {
  m_WiFiCredentialsCallbackHandler = cb;
}

/**
* @brief Get called when receive authentication configurations
*/
void BLEProvClass::onCloudCredentials(CloudCredentialsCallbackHandler cb) {
  m_CloudCredentialsCallbackHandler = cb;
}

/**
* @brief Get called when BLE provisioning completed
*/
void BLEProvClass::onBleProvDone(BleProvDoneCallbackHandler cb) {
  m_BleProvDoneCallbackHandler = cb;
}

/**
* @brief Get called when BLE MAC required
*/
String BLEProvClass::getBLEMac() {
  return String(NimBLEDevice::getAddress().toString().c_str());
}

/**
* @brief Get called to check whether BLE configuration has finished
*/
bool BLEProvClass::bleConfigDone() {
  return m_provConfigDone;
}

BLEProvClass::~BLEProvClass() {}


