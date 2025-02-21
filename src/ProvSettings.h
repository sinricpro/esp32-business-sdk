/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 */

#pragma once 
 
// #define DEBUG_PROV_LOG   // Print provisioning debug logs

// DO NOT CHANGE !! 
#define DEFAULT_BLE_PROV_TIMEOUT      60000 * 45          // BLE provisioning timeout. Default 45 mins.
#define BLE_HOST_PREFIX               "PROV_"             // mandatory product identification prefix
#define BLE_PROV_VERSION              1                   // provisioning protocol version
#define BLE_FRAGMENT_SIZE             180                 // BLE message size. Capped at 180 because IPhone 8 limitations.
#define PRODUCT_CONFIG_FILE           "/prod_config.json" // product configuration file 
#define BUSINESS_SDK_VERSION          "1.1.2"             // SDK version  