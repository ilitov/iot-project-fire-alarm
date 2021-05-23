#include "MessagesCallbacks.h"

void MasterCallback::operator()(const Message &msg) {
	// TODO: Re-write this function.

	Serial.println("Sending a message to MQTT server:");
	Serial.print("Message author: ");
	char macAddr[7];
	memcpy(macAddr, msg.m_mac, 6);
	macAddr[6] = '\0';
	Serial.println(macAddr);

	Serial.print("Message type: ");
	switch (msg.m_msgType) {
	case MessageType::MSG_ANNOUNCE_NAME:
		Serial.println("MSG_ANNOUNCE_NAME");
		break;
	case MessageType::MSG_SENSOR_DATA:
		Serial.println("MSG_SENSOR_DATA");
		break;
	case MessageType::MSG_STOP_ALARM:
		Serial.println("MSG_SENSOR_DATA");
		break;
	default:
		break;
	}
}

SlaveCallback::SlaveCallback(EspNowManager &espman)
	: m_espman(&espman) {

}

void SlaveCallback::operator()(const Message &msg) {
	if (m_espman) {
		const MessageRaw &transmitMsg = prepareMessageForTransmission(msg);
		m_espman->sendData(reinterpret_cast<const uint8_t*>(&transmitMsg), sizeof(transmitMsg));
	}
}
