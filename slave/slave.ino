#include <WiFi.h>
#include <HTTPClient.h>
#include "Adafruit_Si7021.h"


struct Si7021Data{
    float temperature;
    float humidity;
};

//const char* ssid = "REPLACE_WITH_YOUR_SSID";
//const char* password = "REPLACE_WITH_YOUR_PASSWORD";

const char* ssid = "";
const char* password = "";
const char* serverEndpoint = "https://kpq1zeb4x2.execute-api.eu-west-1.amazonaws.com/poc/";

Adafruit_Si7021 sensor = Adafruit_Si7021();
Si7021Data data = {0.0, 0.0};


void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    Serial.println("Connecting");
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());

    if (!sensor.begin()) {
        Serial.println("Did not find Si7021 sensor!");
        while (true)
           ;
    }
}

void updateSensorData() {
    data.temperature = sensor.readTemperature();
    data.humidity = sensor.readHumidity();
}

void sendDataToLambda() {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
        HTTPClient http;
    
        // Your Domain name with URL path or IP address with path
        http.begin(serverEndpoint);

        // Specify content-type header
        http.addHeader("Content-Type", "application/json");
        // Data to send with HTTP POST
        String temperature = "Temperature=" + String(data.temperature, 2);
        String humidity = "Humidity=" + String(data.humidity, 2);
   
//        String httpRequestData = "{\"phone_number\":\"359879430447\",\"message\":\"" + temperature +" "+ humidity + "\"}";
        String httpRequestData = "{\"phone_number\":\"359879430447\",\"message\":\"test123\"}";           
        // Send HTTP POST request
        int httpResponseCode = http.POST(httpRequestData);

        Serial.println(httpRequestData);

        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
          
        // Free resources
        http.end();
    } else {
        Serial.println("WiFi Disconnected");
    }
}

void loop() {
   
      //Check WiFi connection status
//      if(WiFi.status()== WL_CONNECTED){
//          HTTPClient http;
//      
//          // Your Domain name with URL path or IP address with path
//          http.begin(serverEndpoint);
//
//          // Specify content-type header
//          http.addHeader("Content-Type", "application/json");
//          // Data to send with HTTP POST
//          String httpRequestData = "{\"phone_number\":\"359879430447\",\"message\": \"kompot\"}";           
//          // Send HTTP POST request
//          int httpResponseCode = http.POST(httpRequestData);
//
//          Serial.print("HTTP Response code: ");
//          Serial.println(httpResponseCode);
//            
//          // Free resources
//          http.end();
//      } else {
//          Serial.println("WiFi Disconnected");
//      }

      updateSensorData();
      sendDataToLambda();
      delay(10000);
}
