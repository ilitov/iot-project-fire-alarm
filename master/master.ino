#include <WiFi.h>
#include <esp_wifi.h>
#include "EspNowManager.h"

const unsigned char SlaveMAC[6] = {0x7C,0x9E,0xBD,0xED,0x95,0xF4}; 
const bool isMaster = true;

static EspNowManager &espman = EspNowManager::instance();
static MasterProcessorPeers peersProcessor{MasterCallbackPeers{espman}};

void setupWiFi(){
  WiFi.mode(WIFI_STA);
  WiFi.beginSmartConfig();
  //WiFi.begin("MY_WIF_SSID", "PASSWORD");

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
}

void setup() {
  Serial.begin(115200);
  setupWiFi();

  uint32_t wifiChannel = WiFi.channel();
  Serial.print("WiFi channel: ");
  Serial.println(wifiChannel);

  if(false == espman.init(&peersProcessor, NULL, wifiChannel, isMaster, WiFi.macAddress().c_str())){
    Serial.println("Failed to init ESP-NOW, restarting!");
    esp_restart();
  }
}

void loop() {
  //Serial.println("Do something...");

  delay(15000);
}
