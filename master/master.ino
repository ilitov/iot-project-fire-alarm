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


//Used for parsing telegram command - string
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
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
    Serial.println(text);

    String from_name = bot.messages[i].from_name;
    String text = bot.messages[i].text;

    static int id = 0;

    // /pause <staq> <time>
    if (text == "/pause") {

      String room = getValue(text, " ", 1);
      unsigned long alarmPauseDuration = (unsigned long) (getValue(text, " ", 2).toInt());
      //parse
      Message m;

      m.m_msgId = id++;
      strcpy(m.m_data.stopInformation.macAddress, espman.m_slavesToMacMap[room].c_str());
      MessagesMap::parseMacAddressReadable(WiFi.macAddress().c_str(),m.m_mac);
      m.m_msgType = MessageType::MSG_STOP_ALARM;
      m.m_data.stopInformation.stopDuration = alarmPauseDuration * 1000;
      
      espman.enqueueSendDataAsync(m);
      String welcome = "Successfully stopped alarm for " + alarmPauseDuration + " seconds!";
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
