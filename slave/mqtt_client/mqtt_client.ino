
и#include <WiFi.h>
#include <WiFiClient.h>
#include <MQTT.h> // https://github.com/256dpi/arduino-mqtt

const char* mqtt_server = "tb.genevski.com";
const char* clientId = "";
const char* user = "";
const char* pass = "";

WiFiClient wifi;
MQTTClient client;

void setup_wifi() {
  WiFi.begin("", "kokichev"); 
  Serial.println("Connecting to wifi ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
}

bool reconnect() {
  Serial.println("Reconnect requested");
  if(client.connected()) {
    Serial.println("MQTT client is still connected");
    return true;
  } 
  
  Serial.print("Reconnecting to MQTT server...");  
  if(client.connect(clientId, user, pass)) {
    Serial.println("connected");
    
    //client.subscribe("topics/2");
    //Serial.println("resubscribed");
    return true;
    
  } else {
    Serial.println("failed");
    lwmqtt_err_t err = client.lastError();
    Serial.println(err);

    lwmqtt_return_code_t ret = client.returnCode();
    Serial.println(ret);
    return false;
  }
}

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));
  
  setup_wifi();
  
  client.onMessage(messageReceived);
  client.begin(mqtt_server, wifi);
}

void loop() {
  
  float temperature = random(200, 301) / 10.0;
  float humidity = random(200,301) / 10.0;
  float temperature2 = random(200, 301) / 10.0;
  float humidity2 = random(200,301) / 10.0;
  if(!reconnect()){
    // TODO buffer the measurement to send next time
  } else {
    client.loop(); // process incoming messages and maintain connection to server
    delay(10);
      
    // TODO add sequence number
//    String json = "{\"temperature\":" + String(temperature,1) + "}";
    String json = "{\"temperature-staq1\":" + String(temperature,1) + ",\"humidity-staq1\":" + String(humidity,1)+ "}";
    String json2 = "{\"temperature-staq2\":" + String(temperature2,1) + ",\"humidity-staq2\":" + String(humidity2,1)+ "}";  
    Serial.println(json);
    Serial.println(json2);

    int qos = 1;
    bool retained = false;
    bool success = client.publish("/v1/devices/fire-alarm/telemetry/staq1", json.c_str(), retained, qos);
    bool success1 = client.publish("/v1/devices/fire-alarm/telemetry/staq2", json2.c_str(), retained, qos);
    if(!success || !success1){
      Serial.println("publish failed");
      lwmqtt_err_t err = client.lastError(); // https://github.com/256dpi/arduino-mqtt/blob/master/src/lwmqtt/lwmqtt.h
      Serial.println(err);
    }
  }
  delay(2000);
}
