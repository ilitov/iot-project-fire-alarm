#include <cstring>

#include "MessageLibrary.h"

static void encodeMessageRawData(MessageRaw &result, const Message &msg) {
	switch (msg.m_msgType) {
	case MessageType::MSG_ANNOUNCE_NAME:
		std::memcpy(result.m_data, msg.m_data.name, sizeof(msg.m_data.name));
		break;
	case MessageType::MSG_SENSOR_DATA:
		std::memcpy(result.m_data, &msg.m_data.humidity, sizeof(msg.m_data.humidity));
		std::memcpy(result.m_data + sizeof(msg.m_data.humidity), &msg.m_data.temp, sizeof(msg.m_data.temp));
		break;
	default:
		break;
	}
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
	default:
		break;
	}
}

MessageRaw prepareMessageForTransmission(const Message &msg) {
	MessageRaw result{};
	
	std::memcpy(result.m_mac, msg.m_mac, sizeof(msg.m_mac) * sizeof(msg.m_mac[0]));
	result.m_msgType = static_cast<uint8_t>(msg.m_msgType);
	result.m_alarm = static_cast<uint8_t>(msg.m_msgId);
	result.m_msgId = msg.m_msgId;

	encodeMessageRawData(result, msg);

	return result;
}

Message getTransmittedMessage(const MessageRaw &msg) {
	Message result{};

	std::memcpy(result.m_mac, msg.m_mac, sizeof(msg.m_mac) * sizeof(msg.m_mac[0]));
	result.m_msgType = static_cast<MessageType>(msg.m_msgType);
	result.m_alarm = msg.m_alarm;
	result.m_msgId = msg.m_msgId;

	decodeMessageRawData(result, msg);

	return result;
}
