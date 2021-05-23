#include <cstring>

#include "MessageLibrary.h"

static int encodeMessageRawData(MessageRaw &result, const Message &msg) {
	switch (msg.m_msgType) {
	case MessageType::MSG_ANNOUNCE_NAME:
		std::memcpy(result.m_data, msg.m_data.name, sizeof(msg.m_data.name));
		return sizeof(msg.m_data.name);
	case MessageType::MSG_SENSOR_DATA:
		std::memcpy(result.m_data, &msg.m_data.humidity, sizeof(msg.m_data.humidity));
		std::memcpy(result.m_data + sizeof(msg.m_data.humidity), &msg.m_data.temp, sizeof(msg.m_data.temp));
		return sizeof(msg.m_data.humidity) + sizeof(msg.m_data.temp);
	case MessageType::MSG_STOP_ALARM:
		std::memcpy(result.m_data, msg.m_data.macAddress, sizeof(msg.m_data.macAddress));
		return sizeof(msg.m_data.macAddress);
	default:
		break;
	}
	
	return 0;
}

static void decodeMessageRawData(Message &result, const MessageRaw &msg) {
	switch (result.m_msgType) {
	case MessageType::MSG_ANNOUNCE_NAME:
		std::memcpy(result.m_data.name, msg.m_data, sizeof(result.m_data.name));
		break;
	case MessageType::MSG_SENSOR_DATA:
		std::memcpy(&result.m_data.humidity, msg.m_data, sizeof(result.m_data.humidity));
		std::memcpy(&result.m_data.temp, msg.m_data + sizeof(result.m_data.humidity), sizeof(result.m_data.temp));
		break;
	case MessageType::MSG_STOP_ALARM:
		std::memcpy(result.m_data.macAddress, msg.m_data, sizeof(result.m_data.macAddress));
		break;
	default:
		break;
	}
}

int prepareMessageForTransmission(const Message &msg, MessageRaw &result) {
	
	std::memcpy(result.m_mac, msg.m_mac, sizeof(msg.m_mac) * sizeof(msg.m_mac[0]));
	result.m_msgType = static_cast<uint8_t>(msg.m_msgType);
	result.m_alarmStatus = static_cast<uint8_t>(msg.m_alarmStatus);
	result.m_msgId = msg.m_msgId;

	return sizeof(MessageRawBase) + encodeMessageRawData(result, msg);
}

Message getTransmittedMessage(const MessageRaw &msg) {
	Message result{};

	std::memcpy(result.m_mac, msg.m_mac, sizeof(msg.m_mac) * sizeof(msg.m_mac[0]));
	result.m_msgType = static_cast<MessageType>(msg.m_msgType);
	result.m_alarmStatus = static_cast<AlarmType>(msg.m_alarmStatus);
	result.m_msgId = msg.m_msgId;

	decodeMessageRawData(result, msg);

	return result;
}
