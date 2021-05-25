#include <WiFi.h>
#include "EspNowManager.h"

const unsigned char SlaveMAC[6] = {0x7C,0x9E,0xBD,0xED,0x95,0xF4}; 
const bool isMaster = true;

static EspNowManager &espman = EspNowManager::instance();
static MasterProcessorPeers peersProcessor{MasterCallbackPeers{espman}};

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  //uint32_t wifiChannel = WiFi.channel();
  uint32_t wifiChannel = 0;
  Serial.print("WiFi channel: ");
  Serial.println(wifiChannel);

  if(false == espman.init(&peersProcessor, NULL, wifiChannel, isMaster, WiFi.macAddress().c_str())){
    esp_restart();
  }
}

void loop() {
  Serial.println("Do something...");

  delay(500);
}
