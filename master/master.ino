#include <WiFi.h>
#include <WebServer.h>
#include <esp_wifi.h>

#include "EspNowManager.h"
#include "EspSettings.h"
#include "LEDBlink.h"

const unsigned char SlaveMAC[6] = {0x7C,0x9E,0xBD,0xED,0x95,0xF4}; 
const bool isMaster = true;

static EspNowManager &espman = EspNowManager::instance();
static MasterProcessorPeers peersProcessor{MasterCallbackPeers{espman}};

static ESPSettings &espSettings = ESPSettings::instance();

void setupWiFi(){
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.beginSmartConfig();

  // Configure the access to WiFi from the smartphone(app: EspTouch: SmartConfig)
  Serial.println("Waiting for SmartConfig.");
  while (!WiFi.smartConfigDone()) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("SmartConfig received.");

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // This is needed if we are not in AP mode.
  WiFi.setSleep(WIFI_PS_NONE);
  //esp_wifi_set_ps(WIFI_PS_NONE);

  // This channel must be sent to slaves through the WebServer.
  uint32_t wifiChannel = WiFi.channel();
  Serial.print("WiFi channel: ");
  Serial.println(wifiChannel);

  if(!espSettings.setESPNowChannel(wifiChannel)){
    Serial.println("The WiFi channel is not appropriate! Restarting...");
    esp_restart();
  }

  // Save the settings to the internal memory.
  espSettings.updateSettings();
}

void setup() {
  Serial.begin(115200);

  LEDBlink setupLED(2); // LED pin = 2
  setupLED.start();
  
  // Setup the file system and ESP settings.
  if(!espSettings.setup()){
    esp_restart();
  }

  // Setup a phone number and credentials.
  espSettings.setupUserSettings();

  // Onboard the ESP with SmartConfig.
  setupWiFi();

  if(false == espman.init(&peersProcessor, NULL, espSettings.getESPNowChannel(), isMaster, WiFi.macAddress().c_str())){
    Serial.println("Failed to init ESP-NOW, restarting!");
    esp_restart();
  }

  // Stop the LED.
  setupLED.stop();

  Serial.println("setup() completed.");
}

void loop() {
  //Serial.println("Do something...");

  delay(15000);
}
