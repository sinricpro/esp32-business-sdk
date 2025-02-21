/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 */

#include "BLEProv.h"

/**
 * @brief BLEProvClass constuctor.
 */
BLEProvClass::BLEProvClass() 
: m_begin(false)
, m_WiFiCredentialsCallbackHandler(nullptr),
  m_CloudCredentialsCallbackHandler(nullptr),
  m_receivedCloudCredentialsConfig(""),
  m_uuidService(BLE_SERVICE_UUID),
  m_uuidWiFiConfig(BLE_WIFI_CONFIG_UUID),
  m_uuidWiFiConfigNotify(BLE_WIFI_CONFIG_NOTIFY_UUID),
  m_uuidKeyExchange(BLE_KEY_EXCHANGE_UUID),
  m_uuidKeyExchangeNotify(BLE_KEY_EXCHANGE_NOTIFY_UUID),
  m_uuidCloudCredentialConfig(BLE_CLOUD_CREDENTIAL_CONFIG_UUID),  
  m_uuidCloudCredentialConfigNotify(BLE_CLOUD_CREDENTIAL_CONFIG_NOTIFY_UUID),
  m_uuidWiFiList(BLE_WIFI_LIST_UUID),
  m_uuidWiFiListNotify(BLE_WIFI_LIST_NOTIFY_UUID),
  m_uuidProvInfo(BLE_PROV_INFO_UUID),
  m_uuidProvInfoNotify(BLE_INFO_NOTIFY_UUID){}

/**
* @brief Setup BLE provisioning endpoints 
*/
void BLEProvClass::begin(const String &deviceName, const String &retailItemId) {
  if (m_begin) stop();

  DEBUG_PROV(PSTR("[BLEProvClass.begin]: Setup BLE endpoints ..\r\n"));
  
  NimBLEDevice::init(deviceName.c_str());
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); 
  NimBLEDevice::setMTU(512);
   
  m_pServer = NimBLEDevice::createServer();
  m_pServer->setCallbacks(this);
  m_pServer->advertiseOnDisconnect(false);

  m_pService = m_pServer->createService(m_uuidService);

  m_provWiFiConfig = m_pService->createCharacteristic(m_uuidWiFiConfig, NIMBLE_PROPERTY::WRITE_NR); 
  m_provWiFiConfig->setValue("wifi_config");
  m_provWiFiConfig->setCallbacks(this);

  m_provWiFiConfigNotify = m_pService->createCharacteristic(m_uuidWiFiConfigNotify, NIMBLE_PROPERTY::NOTIFY); 
  m_provWiFiConfigNotify->setValue("wifi_config_notify");
  m_provWiFiConfigNotify->setCallbacks(this);

  m_provKeyExchange = m_pService->createCharacteristic(m_uuidKeyExchange, NIMBLE_PROPERTY::WRITE_NR); 
  m_provKeyExchange->setValue("key_exchange");
  m_provKeyExchange->setCallbacks(this); 

  m_provKeyExchangeNotify = m_pService->createCharacteristic(m_uuidKeyExchangeNotify, NIMBLE_PROPERTY::NOTIFY); 
  m_provKeyExchangeNotify->setValue("key_exchange_notify");
  m_provKeyExchangeNotify->setCallbacks(this); 

  m_provCloudCredentialConfig = m_pService->createCharacteristic(m_uuidCloudCredentialConfig, NIMBLE_PROPERTY::WRITE); 
  m_provCloudCredentialConfig->setValue("cloud_credential_config");
  m_provCloudCredentialConfig->setCallbacks(this); 

  m_provCloudCredentialConfigNotify = m_pService->createCharacteristic(m_uuidCloudCredentialConfigNotify, NIMBLE_PROPERTY::NOTIFY); 
  m_provCloudCredentialConfigNotify->setValue("cloud_credential_config_notify");
  m_provCloudCredentialConfigNotify->setCallbacks(this);

  m_provWiFiList = m_pService->createCharacteristic(m_uuidWiFiList, NIMBLE_PROPERTY::WRITE_NR); 
  m_provWiFiList->setValue("wifi_list");
  m_provWiFiList->setCallbacks(this); 

  m_provWiFiListNotify = m_pService->createCharacteristic(m_uuidWiFiListNotify, NIMBLE_PROPERTY::NOTIFY); 
  m_provWiFiListNotify->setValue("wifi_list_notify");
  m_provWiFiListNotify->setCallbacks(this);

  m_provInfo = m_pService->createCharacteristic(m_uuidProvInfo, NIMBLE_PROPERTY::WRITE_NR); 
  m_provInfo->setValue("prov_info");
  m_provInfo->setCallbacks(this); 

  m_provInfoNotify = m_pService->createCharacteristic(m_uuidProvInfoNotify, NIMBLE_PROPERTY::NOTIFY); 
  m_provInfoNotify->setValue("prov_info_notify");
  m_provInfoNotify->setCallbacks(this);

 
  m_pService->start();
  
  m_pAdvertising = NimBLEDevice::getAdvertising();
  //m_pAdvertising->setScanResponse(true); // must be true or else BLE name gets truncated
  m_pAdvertising->setName(deviceName.c_str());
  m_pAdvertising->addServiceUUID(m_uuidService);
  m_pAdvertising->enableScanResponse(true);
  m_pAdvertising->start(); 

  DEBUG_PROV(PSTR("[BLEProvClass.begin]: done!\r\n"));
  m_retailItemId = retailItemId;  
  m_begin = true;
}

/**
* @brief Generate a session encryption key using public key.
*/
void BLEProvClass::handleKeyExchange(const std::string& publicKey, NimBLECharacteristic* pCharacteristic) {
  DEBUG_PROV(PSTR("[BLEProvClass.handleKeyExchange()]:: Start!\r\n"));

  struct KeyExchangeData {
    BLEProvClass* provClass;
    const char* publicKey;
  };

  KeyExchangeData* data = (KeyExchangeData*)malloc(sizeof(KeyExchangeData));
  if (data == nullptr) {
    DEBUG_PROV(PSTR("[BLEProvClass.handleKeyExchange()]: ProvData allocation failed!\r\n"));
    return;
  }

  data->provClass = this;
  data->publicKey = publicKey.c_str();

  void (*onCharacteristicWriteTask)(void*) = [](void* param) {
    KeyExchangeData* data = static_cast<KeyExchangeData*>(param);
    std::string sessionKey;
    std::string publicKey(data->publicKey);

    if(data->provClass->m_crypto.initMbedTLS()) {
     data->provClass->m_crypto.getSharedSecret(publicKey, sessionKey);      
    }    

    data->provClass->m_crypto.deinitMbedTLS();

    DEBUG_PROV(PSTR("[BLEProvClass.handleKeyExchange()]: Encrypted session key is: %s\r\n"), sessionKey.c_str());      

    data->provClass->splitWrite(data->provClass->m_provKeyExchangeNotify, sessionKey);
    
    vTaskDelete(NULL);
  };

  // Needs to run in a RTOS Task because MbedTLS crypto stack needs a large stack 
  xTaskCreatePinnedToCore(onCharacteristicWriteTask, "BLEProvCharacteristicTask", 12288, data, 0, NULL, 0); 
}

 
/**
* @brief Called when mobile sends Sinric Pro credentials (appkey, secret, devceids). 
* Mobile sends authentication config string in chucks due to BLE limitations
*/
void BLEProvClass::handleCloudCredentialsConfig(const std::string& cloudCredentialsConfigChuck, NimBLECharacteristic* pCharacteristic) {
  if(m_expectedAuthConfigPayloadSize == -1) {
      m_expectedAuthConfigPayloadSize = std::atoi(cloudCredentialsConfigChuck.c_str());
      DEBUG_PROV(PSTR("[BLEProvClass.handleCloudCredentialsConfig()]: Expected config payload size: %d\r\n"), m_expectedAuthConfigPayloadSize);
  } else {
    // Append data chucks
    m_receivedCloudCredentialsConfig.append(cloudCredentialsConfigChuck);
    DEBUG_PROV(PSTR("[BLEProvClass.handleCloudCredentialsConfig()]: %d/%d\r\n"), m_receivedCloudCredentialsConfig.size(), m_expectedAuthConfigPayloadSize);
    
    if(m_receivedCloudCredentialsConfig.size() == m_expectedAuthConfigPayloadSize) {
      m_expectedAuthConfigPayloadSize = -1;
      DEBUG_PROV(PSTR("[BLEProvClass.handleCloudCredentialsConfig()]: Auth config payload receive completed\r\n")); 
      
      if (m_CloudCredentialsCallbackHandler) {
         std::vector<uint8_t> decodedConfig = m_crypto.base64Decode(m_receivedCloudCredentialsConfig);
         m_crypto.aesCTRXdecrypt(m_crypto.key, m_crypto.iv, decodedConfig);
         std::string authConfig(decodedConfig.begin(), decodedConfig.end());    
    
         DEBUG_PROV(PSTR("[BLEProvClass.handleCloudCredentialsConfig()]: Decrypted config: %s\r\n"), authConfig.c_str());  
    
         // Calling callback to connect to WiFi      
         bool success = m_CloudCredentialsCallbackHandler(String(authConfig.c_str())); 
         std::string jsonString;
         
         JsonDocument doc;
         doc["success"] = success ? true : false;
         serializeJson(doc, jsonString); 
         DEBUG_PROV(PSTR("[BLEProvClass.handleCloudCredentialsConfig()]: Response: %s\r\n"), jsonString.c_str());    
    
         splitWrite(m_provCloudCredentialConfigNotify, jsonString);

         DEBUG_PROV(PSTR("[BLEProvClass.handleCloudCredentialsConfig()]: Notified!\r\n"));    

         // Wait until client gets the response before we wrap up.
         ProvUtil::wait(2000);
      
         m_provConfigDone = true;
          
         if(success && m_BleProvDoneCallbackHandler) {
            m_BleProvDoneCallbackHandler();
         }
        } else {
          DEBUG_PROV(PSTR("[BLEProvClass.handleCloudCredentialsConfig()]: Auth callback not defined!\r\n"));  
          
          std::string jsonString;
          JsonDocument doc;
          doc[F("success")] = false;
          doc[F("message")] = F("Failed set authentication (nocallback)..");
          serializeJson(doc, jsonString);
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
  DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()]: Start!\r\n"));  
 
  if (m_WiFiCredentialsCallbackHandler) {
     std::vector<uint8_t> decoded = m_crypto.base64Decode(wificonfig);
     m_crypto.aesCTRXdecrypt(m_crypto.key, m_crypto.iv, decoded);
     std::string wiFi_config(decoded.begin(), decoded.end());

     DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()]: Wi-Fi config: %s\r\n"), wiFi_config.c_str());  
     
     bool success = m_WiFiCredentialsCallbackHandler(String(wiFi_config.c_str())); 

     std::string jsonString = "";
     
     if(success) {
        JsonDocument doc;
        doc[F("success")] = true;
        doc[F("message")] = F("Success!");
        doc[F("bssid")] = WiFi.macAddress();
        doc[F("ip")] = WiFi.localIP().toString();
        serializeJson(doc, jsonString); 
     } else {
        JsonDocument doc;
        doc[F("success")] = false;
        doc[F("message")] = F("Failed to connect to WiFi. Is password correct?");
        serializeJson(doc, jsonString);
     }

     DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()]: WiFi Config response size: %u\r\n"), jsonString.length());    
     DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()]: WiFi Config response: %s\r\n"), jsonString.c_str());    
      
     splitWrite(m_provWiFiConfigNotify, jsonString);

     DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()]: Done!\r\n"));          
    } else {
      DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()]: m_WiFiCredentialsCallbackHandler not set!\r\n"));    
      
      std::string jsonString;
      JsonDocument doc;
      doc[F("success")] = false;
      doc[F("message")] = F("Wifi Credentials Callback not set!..");
      serializeJson(doc, jsonString);
      m_provWiFiConfigNotify->setValue(jsonString);
      m_provWiFiConfigNotify->notify();
   }

   DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()]: End!\r\n"));   
}

/**
* @brief Called when mobile wants a list of WiFis ESP can connect to.
*/
void BLEProvClass::handleWiFiList(NimBLECharacteristic* pCharacteristic) {
  DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiList()]: Start!\r\n"));  

  DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiList()]: Scanning networks..!\r\n")); 
  
  int scanAttempts = 0;
  const int maxAttempts = 3;
  int ret = 0;

  while (scanAttempts < maxAttempts) {
    WiFi.scanNetworks(false);
    ret = WiFi.scanComplete();
    
    while (ret == WIFI_SCAN_RUNNING) {
      ret = WiFi.scanComplete();
      delay(50);
    }
    
    DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiList()]: Scanning completed..!\r\n"));
    
    if (ret == WIFI_SCAN_FAILED) {
      DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiList()]: Scan failed!\r\n"));
      scanAttempts++;
      if (scanAttempts < maxAttempts) {
        DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiList()]: Resetting WiFi and retrying scan...\r\n"));
        
        // Reset WiFi
        WiFi.mode(WIFI_OFF);   // Turn WiFi off
        delay(500);            // Wait
        WiFi.mode(WIFI_STA);   // Turn WiFi back on
        delay(500);            // Wait for WiFi to initialize
      }
    } else {
      // Scan was successful, break the loop
      break;
    }
  }

  if (scanAttempts == maxAttempts) {
    DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiList()]: All scan attempts failed after WiFi resets!\r\n"));
  } else {
    DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiList()]: Scan successful!\r\n"));
  }

  String jsonString = "[";
  for (int i = 0; i < ret; ++i) {
      if(i != 0) jsonString += ",";
      jsonString += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
      jsonString += "\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  jsonString += "]";

  DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiList()]: WiFi list: %s\r\n"), jsonString.c_str());

  // Free memory
  WiFi.scanDelete();
   
  splitWrite(m_provWiFiListNotify, std::string(jsonString.c_str()));      
      
  DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiList()]: End!\r\n"));    
}

/**
* @brief Split the data into chunks and write. App will reassemble the complete data from these fragments.
*/
void BLEProvClass::splitWrite(NimBLECharacteristic * pCharacteristic, const std::string& data) {
  // Write length
  pCharacteristic->setValue(ProvUtil::to_string(data.length()));
  pCharacteristic->notify(true);
  delay(500);

  // // Write length
  // pCharacteristic->setValue("Hello World");
  // pCharacteristic->notify();

  // delay(10);

  

  // Write data
  int offset          = 0;
  int remainingLength = data.length();
  const uint8_t* str  = reinterpret_cast<const uint8_t*>(data.c_str());

  while (remainingLength > 0) {
    int bytesToSend = min(BLE_FRAGMENT_SIZE, remainingLength); // send in chunks bytes until all the bytes are sent
    DEBUG_PROV(PSTR("[BLEProvClass.splitWrite()]: Sending %u bytes!\r\n"), bytesToSend);    
    pCharacteristic->setValue((str + offset), bytesToSend);
    pCharacteristic->notify();
    delay(10);
    remainingLength -= bytesToSend;
    offset += bytesToSend;
  }

    // Write length
    // pCharacteristic->setValue("Hello World Hello World");
    // pCharacteristic->notify();
}

/**
* @brief Called when mobile wants a information about this device.
*/
void BLEProvClass::handleProvInfo(NimBLECharacteristic* pCharacteristic) {
  DEBUG_PROV(PSTR("[BLEProvClass.handleProvInfo()]: Start!\r\n"));  

  std::string jsonString;
  JsonDocument doc;
  doc[F("retailItemId")] = m_retailItemId;
  doc[F("version")] = BLE_PROV_VERSION;
       
  serializeJson(doc, jsonString);

  DEBUG_PROV(PSTR("[BLEProvClass.handleProvInfo()]: Write: %s\r\n"), jsonString.c_str());  
  
  splitWrite(m_provInfoNotify, jsonString); 

  DEBUG_PROV(PSTR("[BLEProvClass.handleProvInfo()]: End!\r\n"));    
}

void BLEProvClass::onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
  DEBUG_PROV(PSTR("[BLEProvClass.onWrite()]: UUID: %s, Got: %s\r\n"), pCharacteristic->getUUID().toString().c_str(), pCharacteristic->getValue().c_str());
  if (pCharacteristic == m_provKeyExchange && m_provKeyExchange->getLength()) { 
     handleKeyExchange(m_provKeyExchange->getValue(), pCharacteristic); 
  }        
  else if (pCharacteristic == m_provWiFiConfig && m_provWiFiConfig->getLength()) { 
    handleWiFiConfig(m_provWiFiConfig->getValue(), pCharacteristic);
  }
  else if (pCharacteristic == m_provCloudCredentialConfig && m_provCloudCredentialConfig->getLength()) { 
    handleCloudCredentialsConfig(m_provCloudCredentialConfig->getValue(), pCharacteristic);
  }
  else if (pCharacteristic == m_provWiFiList) { 
    handleWiFiList(pCharacteristic);
  }
  else if (pCharacteristic == m_provInfo) { 
    handleProvInfo(pCharacteristic);
  } else {
    DEBUG_PROV(PSTR("[BLEProvClass.onWrite()]: Characteristic not found!"));
  }     
}

/**
* @brief Called when client connect.
*/
void BLEProvClass::onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
  DEBUG_PROV(PSTR("[BLEProvClass.onConnect()]: connection ID: %u\n"), connInfo.getConnHandle());
}
 
/**
* @brief Called when client disconnect..
*/
void BLEProvClass::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
  DEBUG_PROV(PSTR("[BLEProvClass.onDisconnect()]: Client disconnected\r\n"));

  if (m_begin) { 
    DEBUG_PROV(PSTR("[BLEProvClass.onDisconnect()]: Start advertising\r\n"));
    m_pAdvertising->start();
  } 
}

/**
* @brief Called when client request for different MTU..
*/
void BLEProvClass::onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo) {
  DEBUG_PROV(PSTR("[BLEProvClass.onMTUChange()]: MTU updated: %u for connection ID: %u\r\n"), MTU, connInfo.getConnHandle());
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


