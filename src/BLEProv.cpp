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
  m_receivedCloudCredentialsConfig(""){}

/**
* @brief Setup BLE provisioning endpoints 
*/
void BLEProvClass::begin(const String &deviceName, const String &retailItemId) {
  if (m_begin) stop();

  DEBUG_PROV(PSTR("[BLEProvClass.begin]: Setup BLE endpoints ..\r\n"));
  
  BLEDevice::init(deviceName.c_str());
  BLEDevice::setPower(ESP_PWR_LVL_P9);
  BLEDevice::setMTU(512);
   
  m_pServer = BLEDevice::createServer();
  m_pServer->setCallbacks(this);

  m_pService = m_pServer->createService(BLE_SERVICE_UUID);

  m_provInfo = m_pService->createCharacteristic(BLE_PROV_INFO_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE); 
  m_provInfo->setCallbacks(this); 

  m_provInfoNotify = m_pService->createCharacteristic(BLE_PROV_INFO_NOTIFY_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY); 
  m_provInfoNotify->setCallbacks(this);
  m_provWiFiConfigNotify->addDescriptor(new BLE2902());
  
  m_provWiFiConfig = m_pService->createCharacteristic(BLE_WIFI_CONFIG_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE); 
  m_provWiFiConfig->setCallbacks(this);

  m_provWiFiConfigNotify = m_pService->createCharacteristic(BLE_WIFI_CONFIG_NOTIFY_UUID, BLECharacteristic::PROPERTY_NOTIFY); 
  m_provWiFiConfigNotify->setCallbacks(this);
  m_provWiFiConfigNotify->addDescriptor(new BLE2902());

  m_provKeyExchange = m_pService->createCharacteristic(BLE_KEY_EXCHANGE_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE); 
  m_provKeyExchange->setCallbacks(this); 

  m_provKeyExchangeNotify = m_pService->createCharacteristic(BLE_KEY_EXCHANGE_NOTIFY_UUID, BLECharacteristic::PROPERTY_NOTIFY); 
  m_provKeyExchangeNotify->setCallbacks(this); 
  m_provWiFiConfigNotify->addDescriptor(new BLE2902());

  m_provCloudCredentialConfig = m_pService->createCharacteristic(BLE_CLOUD_CREDENTIAL_CONFIG_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE); 
  m_provCloudCredentialConfig->setCallbacks(this); 

  m_provCloudCredentialConfigNotify = m_pService->createCharacteristic(BLE_CLOUD_CREDENTIAL_CONFIG_NOTIFY_UUID, BLECharacteristic::PROPERTY_NOTIFY); 
  m_provCloudCredentialConfigNotify->setCallbacks(this);
  m_provWiFiConfigNotify->addDescriptor(new BLE2902());

  m_provWiFiList = m_pService->createCharacteristic(BLE_WIFI_LIST_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE); 
  m_provWiFiList->setCallbacks(this); 

  m_provWiFiListNotify = m_pService->createCharacteristic(BLE_WIFI_LIST_NOTIFY_UUID, BLECharacteristic::PROPERTY_NOTIFY); 
  m_provWiFiListNotify->setCallbacks(this);
  m_provWiFiConfigNotify->addDescriptor(new BLE2902());

  m_pService->start();
  
  m_pAdvertising = BLEDevice::getAdvertising();
  m_pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
  m_pAdvertising->setScanResponse(true); // must be true or else BLE name gets truncated
  m_pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  m_pAdvertising->setMinPreferred(0x12);  
  m_pAdvertising->start(); 

  DEBUG_PROV(PSTR("[BLEProvClass.begin]: done!\r\n"));
  m_retailItemId = retailItemId;  
  m_begin = true;
}

/**
* @brief Generate a session encryption key using public key.
*/
void BLEProvClass::handleKeyExchange(const std::string& publicKey, BLECharacteristic* pCharacteristic) {
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
void BLEProvClass::handleCloudCredentialsConfig(const std::string& cloudCredentialsConfigChuck, BLECharacteristic* pCharacteristic) {
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
         serializeJsonPretty(doc, jsonString); 
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
          pCharacteristic->setValue((uint8_t *)jsonString.c_str(), jsonString.length());
          pCharacteristic->notify(true);
        }    
    }          
  }     
}  

/**
* @brief Called when mobile sends WiFi credentials.
*/
void BLEProvClass::handleWiFiConfig(const std::string& wificonfig, BLECharacteristic* pCharacteristic) {
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
        serializeJsonPretty(doc, jsonString); 
     } else {
        JsonDocument doc;
        doc[F("success")] = false;
        doc[F("message")] = F("Failed to connect to WiFi. Is password correct?");
        serializeJsonPretty(doc, jsonString);
     }

     DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()]: res size: %u\r\n"), jsonString.length());    
     DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()]: res: %s\r\n"), jsonString.c_str());    
      
     splitWrite(m_provWiFiConfigNotify, jsonString);

     DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()]: Done!\r\n"));          
    } else {
      DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()]: m_WiFiCredentialsCallbackHandler not set!\r\n"));    
      
      std::string jsonString;
      JsonDocument doc;
      doc[F("success")] = false;
      doc[F("message")] = F("Wifi Credentials Callback not set!..");
      serializeJsonPretty(doc, jsonString);
      m_provWiFiConfigNotify->setValue((uint8_t*)jsonString.c_str(), jsonString.length());
      m_provWiFiConfigNotify->notify();
   }

   DEBUG_PROV(PSTR("[BLEProvClass.handleWiFiConfig()]: End!\r\n"));   
}

/**
* @brief Called when mobile wants a list of WiFis ESP can connect to.
*/
void BLEProvClass::handleWiFiList(BLECharacteristic* pCharacteristic) {
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
void BLEProvClass::splitWrite(BLECharacteristic * pCharacteristic, const std::string& data) {
  // Write length
  std::string lengthStr = std::to_string(data.length());
  pCharacteristic->setValue((uint8_t *)lengthStr.c_str(), lengthStr.length());
  pCharacteristic->notify(true);
  delay(500);

  // Write data
  int offset          = 0;
  int remainingLength = data.length();
  uint8_t* str  = (uint8_t *)data.c_str();
  while (remainingLength > 0) {
    int bytesToSend = min(BLE_FRAGMENT_SIZE, remainingLength); // send in chunks bytes until all the bytes are sent
    DEBUG_PROV(PSTR("[BLEProvClass.splitWrite()]: Sending %u bytes!\r\n"), bytesToSend);    
    pCharacteristic->setValue(str + offset, bytesToSend);
    pCharacteristic->notify();
    delay(10);
    remainingLength -= bytesToSend;
    offset += bytesToSend;
  }
}

/**
* @brief Called when mobile wants a information about this device.
*/
void BLEProvClass::handleProvInfo(BLECharacteristic* pCharacteristic) {
  DEBUG_PROV(PSTR("[BLEProvClass.handleProvInfo()]: Start!\r\n"));  

  std::string jsonString;
  JsonDocument doc;
  doc[F("retailItemId")] = m_retailItemId;
  doc[F("version")] = BLE_PROV_VERSION;
       
  serializeJsonPretty(doc, jsonString);

  DEBUG_PROV(PSTR("[BLEProvClass.handleProvInfo()]: Write: %s\r\n"), jsonString.c_str());  
  
  splitWrite(m_provInfoNotify, jsonString); 

  DEBUG_PROV(PSTR("[BLEProvClass.handleProvInfo()]: End!\r\n"));    
}

void BLEProvClass::onWrite(BLECharacteristic* pCharacteristic) {
  DEBUG_PROV(PSTR("[BLEProvClass.onWrite()]: UUID: %s, Got: %s\r\n"), pCharacteristic->getUUID().toString().c_str(), pCharacteristic->getValue().c_str());
  // TODO: Fix later

  // if (pCharacteristic == m_provKeyExchange && m_provKeyExchange->getDataLength()) { 
  //    handleKeyExchange(m_provKeyExchange->getValue(), pCharacteristic); 
  // }        
  // else if (pCharacteristic == m_provWiFiConfig && m_provWiFiConfig->getDataLength()) { 
  //   handleWiFiConfig(m_provWiFiConfig->getValue(), pCharacteristic);
  // }
  // else if (pCharacteristic == m_provCloudCredentialConfig && m_provCloudCredentialConfig->getDataLength()) { 
  //   handleCloudCredentialsConfig(m_provCloudCredentialConfig->getValue(), pCharacteristic);
  // }
  // else if (pCharacteristic == m_provWiFiList) { 
  //   handleWiFiList(pCharacteristic);
  // }
  // else if (pCharacteristic == m_provInfo) { 
  //   handleProvInfo(pCharacteristic);
  // } else {
  //   DEBUG_PROV(PSTR("[BLEProvClass.onWrite()]: Characteristic not found!\r\n"));
  // }     
}

/**
* @brief Called when client connect.
*/
void BLEProvClass::onConnect(BLEServer* pServer) {
  DEBUG_PROV(PSTR("[BLEProvClass.onConnect()]: Client connected\r\n"));
}

// /**
// * @brief Show connected client MTU.
// */
// void BLEProvClass::onConnect(BLEServer* pServer) {
//   //DEBUG_PROV(PSTR("[BLEProvClass.onConnect()]: MTU of client: %d\r\n"), pServer->getPeerMTU(desc->conn_handle));
// }

/**
* @brief Called when client disconnect..
*/
void BLEProvClass::onDisconnect(BLEServer* pServer) {
  DEBUG_PROV(PSTR("[BLEProvClass.onDisconnect()]: Client disconnected\r\n"));

  if (m_begin) { 
    DEBUG_PROV(PSTR("[BLEProvClass.onDisconnect()]: Start advertising\r\n"));
    m_pAdvertising->start();
  } 
}

/**
* @brief Called when client request for different MTU..
*/
void BLEProvClass::onMTUChange(uint16_t MTU) {
  DEBUG_PROV(PSTR("[BLEProvClass.onMTUChange()]: MTU updated: %u\r\n"), MTU);
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
  BLEDevice::deinit();
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
  return String(BLEDevice::getAddress().toString().c_str());
}

/**
* @brief Get called to check whether BLE configuration has finished
*/
bool BLEProvClass::bleConfigDone() {
  return m_provConfigDone;
}

BLEProvClass::~BLEProvClass() {}


