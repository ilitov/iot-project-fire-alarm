#include <WiFi.h>
#include "EspNowManager.h"

const unsigned char MasterMAC[6] = {0x9C,0x9C,0x1F,0xCA,0xFD,0x7C}; 
const bool isMaster = false;

static EspNowManager &espman = EspNowManager::instance();
static SlaveProcessorPeers peerMessagesProcessor{SlaveCallbackPeers{espman}};
static SlaveProcessorSelf myMessagesProcessor{SlaveCallbackSelf{espman}};

// Functions declarations
void authorizeMyselfToMaster(const char *name, int length);

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  //uint32_t wifiChannel = WiFi.channel();
  uint32_t wifiChannel = 0;
  Serial.print("WiFi channel: ");
  Serial.println(wifiChannel);
  
  if(false == espman.init(&peerMessagesProcessor, &myMessagesProcessor, wifiChannel, isMaster, WiFi.macAddress().c_str())){
    esp_restart();
  }

  // TODO: Automate this with WebServers.
  esp_err_t peerResult = espman.addPeer(MasterMAC);
  if(ESP_OK != peerResult){
    Serial.println(esp_err_to_name(peerResult));
    esp_restart();
  }

  const char *someName = "Slave007";
  authorizeMyselfToMaster(someName, strlen(someName));

  Serial.println("setup() completed!");
}

void authorizeMyselfToMaster(const char *name, int length){
  const unsigned long beginTime = millis();
  const unsigned long timeout = 60 * 1000; // 60 sec

  while(!espman.isMasterAcknowledged() && millis() - beginTime < timeout){
    Message msg{};
    MessagesMap::parseMacAddressReadable(WiFi.macAddress().c_str(), msg.m_mac);
    memcpy(msg.m_data.name, name, length);
    msg.m_msgId = 0;
    msg.m_msgType = MessageType::MSG_ANNOUNCE_NAME;

    Serial.println("Connecting to master...");
    
    espman.enqueueSendDataAsync(msg);
    delay(1000); // delay 1 sec
  }

  if(!espman.isMasterAcknowledged()){
    Serial.println("Master doesn't know about me... :(");
    esp_restart();
  }
}

void sendSensorData(){
  // TODO: Make the whole thing a class.
  static int sensorDataID = 0;

  // Read sensor data
  float temp = 22.f;
  float humidity = 45.f;

  Message msg;
  MessagesMap::parseMacAddressReadable(WiFi.macAddress().c_str(), msg.m_mac);
  msg.m_msgType = MessageType::MSG_SENSOR_DATA;
  msg.m_alarmStatus = AlarmType::ALARM_NO_SMOKE_OFF;
  msg.m_msgId = sensorDataID++;
  msg.m_data.temp = temp;
  msg.m_data.humidity = humidity;

  espman.enqueueSendDataAsync(msg);
}

void loop() {
  Serial.println("Sending sensor data...");
  sendSensorData();
  delay(5000);
}
