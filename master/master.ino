#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <esp_wifi.h>

#include "EspNowManager.h"
#include "EspSettings.h"
#include "LEDBlink.h"
#include "HardReset.h"
#include "EspNetworkAnnouncer.h"

#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>

const bool isMaster = true;

static EspNowManager &espman = EspNowManager::instance();
static MasterProcessorPeers peersProcessor{MasterCallbackPeers{espman}};

static ESPSettings &espSettings = ESPSettings::instance();
static ESPNetworkAnnouncer &networkAnnouncer = ESPNetworkAnnouncer::instance();

WiFiClientSecure client;
// Initialize Telegram BOT(Get from Botfather)
#define BOTtoken "1887265752:AAEB7UvNeanC5WMUb2kle6U6eUG5-Rb-ybo"  
UniversalTelegramBot bot(BOTtoken, client);

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

// Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);

    if (chat_id != String(espSettings.getTelegramChatID())){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/stop") {
      //TODO:
      //1. stop given device
      //2. if success -> send message
      //3. if not send no message
      String welcome = "Use the following commands to control your outputs.\n\n";
      welcome += "/led_on to turn GPIO ON \n";
      welcome += "/led_off to turn GPIO OFF \n";
      welcome += "/state to request current GPIO state \n";
      bot.sendMessage(chat_id, welcome, "");
    }
  }
}

void handleTelegramBot(){
  static Timer timer;
  // Checks for new messages every 1 second.
  int botRequestDelay = 1000;
  if (timer.elapsedTime() > botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    timer.reset();
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
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  
  // Enable network announcing.
  networkAnnouncer.begin();

  if(false == espman.init(&peersProcessor, NULL, isMaster, WiFi.macAddress().c_str())){
    Serial.println("Failed to init ESP-NOW, restarting!");
    esp_restart();
  }

  // Stop the LED.
  setupLED.stop();
  
  Serial.println("setup() completed.");
}

// All function calls in loop() are non-blocking and each should complete relatively fast.
void loop() {
  // TODO: Do something meaningful in case of lost connection.
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("There is no WiFi!");
    //esp_restart();
  }

  Serial.println("Loop...");
  
  espman.update();
  networkAnnouncer.handlePeer();

  handleTelegramBot();
  
  delay(1000);
}
