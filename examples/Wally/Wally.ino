/**
 * @brief Example firmware for two-channel relay controller (Wally) using SinricPro ESP32 Business SDK
 *
 * @note This code supports ESP32 only.
 * @note Change Tools -> Flash Size -> Minimum SPIFF
 * @note To enable ESP32 logs: Tools -> Core Debug Level -> Verbose
 * @note Install NimBLE (v1.4.2), ArduinoJson (v7.0.4) from library manager.
 * @note First time compliation takes longer 
 */

#define PRODUCT_ID        ""      /* Product ID from SinricPro Buiness Portal. */
#define FIRMWARE_VERSION  "1.1.1" /* Your firmware version. Must be above SinricPro.h! */

#include <Arduino.h>
#include <SinricProBusinessSdk.h>

#include "Settings.h"
#include "ConfigStore.h"
#include "WiFiManager.h"
#include "WiFiProvisioningManager.h"
#include "OTAManager.h"
#include "ModuleSettingsManager.h"
#include "HealthManager.h"

#include "SinricPro.h"
#include "SinricProSwitch.h"

// Global variables and objects
DeviceConfig m_config;
ConfigStore m_productConfig(m_config);
WiFiManager m_wifiManager;
WiFiProvisioningManager m_provisioningManager(m_productConfig, m_wifiManager);
ModuleSettingsManager m_moduleSettingsManager(m_wifiManager);
OTAManager m_otaManager;
HealthManager m_healthManager;

unsigned long m_lastHeartbeatMills = 0;

// GPIO for push buttons
static uint8_t gpio_reset = 0;

// GPIO for switches
static uint8_t gpio_switch1 = 32;
static uint8_t gpio_switch2 = 33;

// GPIO for relays
static uint8_t gpio_relay1 = 27;
static uint8_t gpio_relay2 = 14;

// GPIO for status LED
static uint8_t gpio_led = 13;

/* Power statuses*/
bool switch1_power_state = true;
bool switch2_power_state = true;
 
struct WallyLightSwitch {
  const uint8_t pin;
  bool pressed;
};

// Define Wally light switches
WallyLightSwitch switch_1 = {gpio_switch1, false};
WallyLightSwitch switch_2 = {gpio_switch2, false};

void ARDUINO_ISR_ATTR isr(void *arg) {
  WallyLightSwitch *s = static_cast<WallyLightSwitch *>(arg);
  s->pressed = true;
}

/**
 * @brief Clear settings and reboot the device.
 */
void factoryResetAndReboot() {
  m_productConfig.clear();
  m_wifiManager.clear();
  ESP.restart();
}

/**
 * @brief Handles buttons for switch 1, switch 2 and reset
 */
void handleSwitchButtonPress() {
   if (switch_1.pressed) {
    Serial.printf("Switch 1 has been changed\n");
    switch_1.pressed = false;
    
    // Toggle switch 1 power state
    switch1_power_state = !switch1_power_state;
    Serial.printf("Toggle State to %s.\n", switch1_power_state ? "true" : "false");

    if (switch1_power_state) { digitalWrite(gpio_relay1, HIGH); } else { digitalWrite(gpio_relay1, LOW); };

    // Update server
    SinricProSwitch& mySwitch1 = SinricPro[m_config.switch_1_id];
    mySwitch1.sendPowerStateEvent(switch1_power_state);

  } else if (switch_2.pressed) {
    Serial.printf("Switch 2 has been changed\n");
    switch_2.pressed = false;
    
    // Toggle switch 2 power state
    switch2_power_state = !switch2_power_state;
    Serial.printf("Toggle State to %s.\n", switch2_power_state ? "true" : "false");

    if (switch2_power_state) { digitalWrite(gpio_relay2, HIGH); } else { digitalWrite(gpio_relay2, LOW); }

    // Update server
    SinricProSwitch& mySwitch2 = SinricPro[m_config.switch_2_id];
    mySwitch2.sendPowerStateEvent(switch2_power_state);
  }

  // Read GPIO0 (external button to reset)
  if (digitalRead(gpio_reset) == LOW) {  //Push button pressed
    Serial.printf("Reset Button Pressed!\n");
    // Key debounce handling
    delay(100);
    int startTime = millis();
    while (digitalRead(gpio_reset) == LOW) {
      delay(50);
    }
    int endTime = millis();

    if ((endTime - startTime) > 10000) {
      Serial.printf("Reset to factory.\n"); // pressed for more than 10secs, reset all
      factoryResetAndReboot();
    } else if ((endTime - startTime) > 3000) {
      Serial.printf("Restart ESP32.\n");
      ESP.restart();
    }
  }
}

/**
 * @brief Callback function for power state changes
 *
 * This function is called when a power state change is requested for a switch.
 *
 * @param deviceId The ID of the device
 * @param state The new power state (true for on, false for off)
 * @return true if the request was handled properly
 */
bool onPowerState(const String& deviceId, bool& state) {
  if (strcmp(m_config.switch_1_id, deviceId.c_str()) == 0) {
    Serial.printf("[onPowerState()]: Change device: %s, power state changed to %s\r\n", deviceId.c_str(), state ? "on" : "off");
    switch1_power_state = state;
    if (switch1_power_state) { digitalWrite(gpio_relay1, HIGH); } else { digitalWrite(gpio_relay1, LOW); } ;
  } else if (strcmp(m_config.switch_2_id, deviceId.c_str()) == 0) {
    Serial.printf("[onPowerState()]: Change device: %s, power state changed to %s\r\n", deviceId.c_str(), state ? "on" : "off");
    switch2_power_state = state;
    if (switch2_power_state) { digitalWrite(gpio_relay2, HIGH); } else { digitalWrite(gpio_relay2, LOW); }
  } else {
    Serial.printf("[onPowerState()]: Device: %s not found!\r\n", deviceId.c_str());
  }

  return true;
}

/**
 * @brief Callback function for setting module settings
 *
 * This function is called when a module setting change is requested.
 *
 * @param id The ID of the setting
 * @param value The new value for the setting
 * @return true if the setting was changed successfully
 */
bool onSetModuleSetting(const String& id, const String& value) {
  SetModuleSettingResult result = m_moduleSettingsManager.handleSetModuleSetting(id, value);
  if (!result.success) {
    SinricPro.setResponseMessage(std::move(result.message));
  }
  return result.success;
}

/**
 * @brief Callback function for OTA updates
 *
 * This function is called when an OTA update is requested.
 *
 * @param url The URL of the update
 * @param major Major version number
 * @param minor Minor version number
 * @param patch Patch version number
 * @param forceUpdate Whether to force the update
 * @return true if the update was successful
 */
bool onOTAUpdate(const String& url, int major, int minor, int patch, bool forceUpdate) {
  OtaUpdateResult result = m_otaManager.handleOTAUpdate(FIRMWARE_VERSION, url, major, minor, patch, forceUpdate);
  if (!result.success) {
    SinricPro.setResponseMessage(std::move(result.message));
  }
  return result.success;
}

/**
 * @brief Set up SinricPro
 *
 * This function initializes SinricPro and sets up the necessary callbacks.
 */
void setupSinricPro() {
  Serial.printf("[setupSinricPro()]: Setup SinricPro.\r\n");

  SinricProSwitch& mySwitch1 = SinricPro[m_config.switch_1_id];
  mySwitch1.onPowerState(onPowerState);

  SinricProSwitch& mySwitch2 = SinricPro[m_config.switch_2_id];
  mySwitch2.onPowerState(onPowerState);

  SinricPro.onConnected([]() {
    Serial.printf("[setupSinricPro()]: Connected to SinricPro\r\n");
  });

  SinricPro.onDisconnected([]() {
    Serial.printf("[setupSinricPro()]: Disconnected from SinricPro\r\n");
  });

  SinricPro.onPong([](uint32_t since) {
    m_lastHeartbeatMills = millis();
  });

  // SinricPro.restoreDeviceStates(true); If you want to restore the last know state from server!

  SinricPro.onReportHealth([&](String& healthReport) {
    return m_healthManager.reportHealth(healthReport);
  });

  SinricPro.onSetSetting(onSetModuleSetting);
  SinricPro.onOTAUpdate(onOTAUpdate);

  SinricPro.begin(m_config.appKey, m_config.appSecret);
}

/**
 * @brief Set up GPIO pins for devices
 *
 * This function should be implemented to set up any necessary GPIO pins.
 */
void setupPins() {
  Serial.printf("[setupPins()]: Setup pin definition.\r\n");
  
  // Configure the input GPIOs
  pinMode(gpio_reset, INPUT);
  pinMode(switch_1.pin, INPUT_PULLUP);
  attachInterruptArg(switch_1.pin, isr, &switch_1, CHANGE);
  pinMode(switch_2.pin, INPUT_PULLUP);
  attachInterruptArg(switch_2.pin, isr, &switch_2, CHANGE);

  // Set the Relays GPIOs as output mode
  pinMode(gpio_relay1, OUTPUT);
  pinMode(gpio_relay2, OUTPUT);
  pinMode(gpio_led, OUTPUT);

  // Write to the GPIOs the default state on booting
  digitalWrite(gpio_led, false);
}

/**
 * @brief Set up SPIFFS file system
 *
 * This function initializes the SPIFFS file system.
 */
void setupSPIFFS() {
  Serial.printf("[setupSPIFFS()]: Setup SPIFFS...\r\n");

  if (SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.printf("[setupSPIFFS()]: done.\r\n");
  } else {
    Serial.printf("[setupSPIFFS()]: fail.\r\n");
  }

  // Serial.println("Erasing SPIFFS...");
  // if (SPIFFS.format()) {
  //   Serial.println("SPIFFS erased successfully.");
  // } else {
  //   Serial.println("Error erasing SPIFFS.");
  // }
}

/**
 * @brief Handle no heartbeat situation
 *
 * This function checks if there's been no heartbeat from the server for a specified interval,
 * and resets the ESP32 if necessary.
 */
void handleNoHeartbeat() {
  unsigned long currentMillis = millis();
  if (currentMillis - m_lastHeartbeatMills >= NO_HEART_BEAT_RESET_INTERVAL) {
    Serial.println("[handleNoHeartbeat()]: No heartbeat for 15 mins. Restarting ESP32...");
    ESP.restart();
  }
}

/**
 * @brief Load product and WiFi configuration
 *
 * This function loads the device configuration and sets up the WiFi connection.
 * If the product is not provisioned, it begins the provisioning process.
 */
void setupConfig() {
  Serial.printf("[setupConfig()]: Loading product & wifi config...\r\n");

  if (m_productConfig.loadConfig()) {    
    if(!m_wifiManager.loadConfig()) {
      Serial.printf("[setupConfig()]: WiFi config load failed. corrupted?...\r\n");
      factoryResetAndReboot();
      return;
    }
  } else {
    // If configuration not loaded, start provisioning process
    Serial.printf("[setupConfig()]: Beginning provisioning...\r\n");

    if (!m_provisioningManager.beginProvision(PRODUCT_ID)) {
      Serial.printf("[setupConfig()]: Provisioning failed. Restarting device.\r\n");
      ESP.restart();  // Restart the ESP if provisioning fails
    }
  } 
}
 
/**
 * @brief Connects to WiFi 
 */
void setupWiFi() {
  Serial.printf("[loadConfigAndSetupWiFi()]: Loading config...\r\n");

  // Set up timeout for WiFi connection attempts
  unsigned long startMillis = millis();

  // Attempt to connect to WiFi
  while (!m_wifiManager.connectToWiFi()) {
    Serial.printf("[loadConfigAndSetupWiFi()]: Cannot connect to WiFi. Retry in 30 seconds!\r\n");
    delay(30000);  // Wait for 30 seconds before retrying
    Serial.printf("[loadConfigAndSetupWiFi()]: Attempting reconnection...\r\n");

    // Check if timeout has been reached
    if ((millis() - startMillis) > WIFI_CONNECTION_TIMEOUT_MS) {
      Serial.printf("[loadConfigAndSetupWiFi()]: Connection retry timeout. Restarting ESP...\r\n");
      ESP.restart();  // Restart the ESP if connection fails after timeout
    }
  }
}

void setup() {
  Serial.begin(BAUDRATE);
  Serial.println();
  delay(1000);

  Serial.printf("Firmware: %s, SinricPro SDK: %s, Business SDK:%s\n", 
                FIRMWARE_VERSION, SINRICPRO_VERSION, BUSINESS_SDK_VERSION);

  setupSPIFFS();
  setupPins();
  setupConfig();
  setupWiFi();
  setupSinricPro();
}

void loop() {
  SinricPro.handle();
  handleNoHeartbeat();
  handleSwitchButtonPress();
  delay(100);
  // Note: Avoid using delay() in the loop. Use non-blocking techniques for timing.
}