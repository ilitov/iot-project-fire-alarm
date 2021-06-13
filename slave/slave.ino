#include <WiFi.h>
#include <esp_wifi.h>

#include "Adafruit_Si7021.h"
#include "alarm.hpp"
#include "mq2.hpp"

#include "EspNowManager.h"
#include "ESPNetworkAnnouncer.h"
#include "EspSettings.h"
#include "HardReset.h"
#include "LEDBlink.h"

const bool isMaster = false;

static EspNowManager &espman = EspNowManager::instance();
static SlaveProcessorPeers peerMessagesProcessor{SlaveCallbackPeers{espman}};
static SlaveProcessorSelf myMessagesProcessor{SlaveCallbackSelf{espman}};

static ESPSettings &espSettings = ESPSettings::instance();
static ESPNetworkAnnouncer &networkAnnouncer = ESPNetworkAnnouncer::instance();

Adafruit_Si7021 si7021 = Adafruit_Si7021();
Alarm theAlarm(14);
MQ2 mq2(4);
int gasThreshhold = 500;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  
  Wire.begin(18,19);
  if (!si7021.begin()) {
        Serial.println("Did not find Si7021 sensor!");
        while (true)
           ;
    }

  LEDBlink setupLED(2); // LED pin = 2
  setupLED.start();

  // Setup the system which handles hard resets.
  HardReset::instance().setup();
  
  // Setup the file system and ESP settings.
  if(!espSettings.init()){
    esp_restart();
  }

  // Setup ESP name and credentials.
  espSettings.setupUserSettings(isMaster);

  // Prepare the system which handles the search for peers.
  networkAnnouncer.begin();
  
  if(false == espman.init(&peerMessagesProcessor, &myMessagesProcessor, isMaster, WiFi.macAddress().c_str())){
    Serial.println("Failed to init ESP-NOW, restarting!");
    esp_restart();
  }

  // Establish connection with master.
  espman.requestMasterAcknowledgement(espSettings.getESPName());
  
  // Stop the LED.
  setupLED.stop();

  Serial.println("setup() completed!");
}

bool alarmSet(int gasValue)
{
  static bool isSet = false;

  if (gasValue > gasThreshhold)
  {
    if (!isSet)
    {
      isSet = true;
      theAlarm.activate();
    }
    return true;
  }
  else
  {
    isSet = false;
    theAlarm.deactivate();
    return false;
  }
}

void sendSensorData(){
  // TODO: Make the whole thing a class.
  static int sensorDataID = 0;

  // Read sensor data
  float temp = si7021.readTemperature();
  float humidity = si7021.readHumidity();
  int gasValue= mq2.readGasConcentration();
  bool alarmIsSet = alarmSet(gasValue);

  Message msg;
  MessagesMap::parseMacAddressReadable(WiFi.macAddress().c_str(), msg.m_mac);
  msg.m_msgType = MessageType::MSG_SENSOR_DATA;
  msg.m_alarmStatus = alarmIsSet ? AlarmType::ALARM_SMOKE_ON : AlarmType::ALARM_NO_SMOKE_OFF;
  msg.m_msgId = sensorDataID++;
  msg.m_data.temp = temp;
  msg.m_data.humidity = humidity;

  if(!espman.enqueueSendDataAsync(msg)){
    Serial.println("Failed to send the message.");
  }
}

void loop() {  
  Serial.println("Sending sensor data...");
  sendSensorData();

  espman.update();
  networkAnnouncer.handlePeer();
  
  delay(1000);
}
