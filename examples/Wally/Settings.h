#pragma once

#define FORMAT_SPIFFS_IF_FAILED             true              /* Format SPIFF if openning failed */
#define WIFI_CONFIG_FILE_NAME               "/wificonfig.dat" /* File name to store wifi configurations  */
#define NO_HEART_BEAT_RESET_INTERVAL        900000          /* If there's no heart-beat ping-pong for 15 mins. reset ESP interval */
#define BAUDRATE                            115200          /* Arduino Serial baud rate */
#define WIFI_CONNECTION_TIMEOUT_MS          1000 * 60 * 10  /* WiFi connection timeout to reset ESP. default: 10 minutes */

// #define ENABLE_DEBUG       /* Enable SinricPro SDK Logs. */
// #define DEBUG_PROV_LOG     /* Print provisioning debug logs */

#ifdef ENABLE_DEBUG
#define DEBUG_ESP_PORT Serial
#define NODEBUG_WEBSOCKETS
#define NDEBUG
#endif


#if !defined(ESP32)
#error "Architecture not supported!"
#endif