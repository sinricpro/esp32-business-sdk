/* 
  Copyright (c) 2019-2024 Sinric
*/

#pragma once 

// DO NOT CHANGE !! 
#define DEFAULT_BLE_PROV_TIMEOUT    60000 * 45            // BLE provisioning timeout. Default 45 mins.
#define BLE_HOST_PREFIX               "PROV_"             // mandatory product identification prefix
#define BLE_PROV_PROTOCOL_VERSION     1                   // provisioning protocol version
#define BLE_FRAGMENT_SIZE             180                 // BLE message size
#define PRODUCT_CONFIG_FILE           "/prod_config.json" // product configuration file 
#define BUSINESS_SDK_VERSION          "1.1.1"             // SDK version  