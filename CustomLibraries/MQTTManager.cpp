#include "MQTTManager.h"

const char *MQTTManager::mqtt_server = "tb.genevski.com";
const char *MQTTManager::clientId = "FireAlarmMaster";
const char *MQTTManager::user = "firealarmmaster";
const char *MQTTManager::pass = "parola123";

MQTTManager::MQTTManager()
	: m_wifiClient()
	, m_client(m_wifiClient) {

}

void MQTTManager::messageReceived(char *topic, uint8_t *payload, unsigned int length) {
	Serial.print("Message received on topic [" + String(topic) + "]");

	for (int i = 0; i < length; ++i) {
		Serial.print((char)payload[i]);
	}

	Serial.println();
}

MQTTManager& MQTTManager::instance() {
	static MQTTManager instance;
	return instance;
}

bool MQTTManager::upload(const Message &msg, const String &room) {
	if (msg.m_msgType != MessageType::MSG_SENSOR_DATA) {
		return false;
	}

	if (!reconnect()) {
		// TODO buffer the measurement to send next time
	}
	else {
		m_client.loop(); // process incoming messages and maintain connection to server
		delay(10);

		const float temperature = msg.m_data.temp;
		const float humidity = msg.m_data.humidity;
		const int gas = msg.m_data.gas;

		const String json = "{\"temperature-"
							+ room + "\":" + String(temperature)
							+ ",\"humidity-"
							+ room + "\":" + String(humidity) +
							+ ",\"gas-"
							+ room + "\":" + String(gas) + "}";

		//Serial.println(json.c_str());

		const bool retained = false;

		const String publishTo = "/v1/devices/fire-alarm/telemetry/" + room;

		const bool success = m_client.publish(publishTo.c_str(), json.c_str(), retained);
		if (!success) {
			Serial.println("publish failed");
		}

		return success;
	}

	return false;
}

bool MQTTManager::reconnect() {
	//Serial.println("Reconnect requested");
	if (m_client.connected()) {
		//Serial.println("MQTT client is still connected");
		return true;
	}

	Serial.print("Reconnecting to MQTT server...");
	if (m_client.connect(clientId, user, pass)) {
		Serial.println("connected");
		return true;
	}

	Serial.println("failed");

	return false;
}

void MQTTManager::begin() {
	m_client.setCallback(messageReceived);
	m_client.setServer(mqtt_server, 1883);
}
