#include <WiFi.h>
#include <WebServer.h>
#include <esp_wifi.h>

#include "EspNowManager.h"
#include "EspSettings.h"
#include "LEDBlink.h"
#include "HardReset.h"
#include "EspNetworkAnnouncer.h"
#include "TelegramBot.h"

const bool isMaster = true;

static EspNowManager &espman = EspNowManager::instance();
static MasterProcessorPeers peersProcessor{MasterCallbackPeers{espman}};

static ESPSettings &espSettings = ESPSettings::instance();
static ESPNetworkAnnouncer &networkAnnouncer = ESPNetworkAnnouncer::instance();

static TelegramBot &telegramBot = TelegramBot::instance();

void setupWiFi(){
  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);
  
  WiFi.beginSmartConfig();

  //Configure the access to WiFi from the smartphone(app: EspTouch: SmartConfig)
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

  WiFi.setSleep(WIFI_PS_NONE);

  // This channel must be sent to slaves through the WebServer.
  const uint32_t wifiChannel = WiFi.channel();
  Serial.print("WiFi channel: ");
  Serial.println(wifiChannel);

  if(espSettings.getESPNowChannel() != wifiChannel){
    Serial.println("Current wifiChannel != wifiChannel from settings.");
    
    if(!espSettings.setESPNowChannel(wifiChannel)){
      Serial.println("The WiFi channel is not appropriate! Restarting...");
      esp_restart();
    }

     // Save the settings to the internal memory.
    espSettings.updateSettings();
  }
  
}

void setup() {
  Serial.begin(115200);

  LEDBlink setupLED(2); // LED pin = 2
  setupLED.start();

  // Setup the system which handles hard resets.
  HardReset::instance().setup();
  
  // Setup the file system and ESP settings.
  if(!espSettings.init()){
    esp_restart();
  }

  // Setup a phone number and credentials.
  espSettings.setupUserSettings(isMaster);

  // Onboard the ESP with SmartConfig.
  setupWiFi();
  
  // Enable network announcing.
  networkAnnouncer.begin();

  if(false == espman.init(&peersProcessor, NULL, isMaster, WiFi.macAddress().c_str())){
    Serial.println("Failed to init ESP-NOW, restarting!");
    esp_restart();
  }

  // Run Telegram bot on a separate thread.
  telegramBot.begin();

  // Stop the LED.
  setupLED.stop();
  
  Serial.println("setup() completed.");
}

// All function calls in loop() are non-blocking and each should complete relatively fast.
void loop() {
	// TODO: Do something meaningful in case of lost connection.
	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("There is no WiFi!");
		//esp_restart();
	}

	Serial.println("Loop...");

	espman.update();
	networkAnnouncer.handlePeer();

	delay(1000);
}
