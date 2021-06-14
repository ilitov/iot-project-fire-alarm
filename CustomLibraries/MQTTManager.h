#ifndef _MQTT_MANAGER_HEADER_
#define _MQTT_MANAGER_HEADER_

#include <WiFiClient.h>
#include <PubSubClient.h>
#include "MessageLibrary.h"

class MQTTManager {
	static const char* mqtt_server;
	static const char* clientId;
	static const char* user;
	static const char* pass;

	MQTTManager();

	static void messageReceived(char *topic, uint8_t *payload, unsigned int length);

	bool reconnect();

public:
	static MQTTManager& instance();

	bool upload(const Message &msg, const String &room);
	void begin();

private:
	WiFiClient m_wifiClient;
	PubSubClient m_client;
};

#endif // !_MQTT_MANAGER_HEADER_
