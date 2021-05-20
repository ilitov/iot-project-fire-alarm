#ifndef _MESSAGE_LIBRARY_
#define _MESSAGE_LIBRARY_

#include <cstdint>

static const int MAX_LEN_ESP_NAME = 32;

enum class MessageType {
	MSG_SENSOR_DATA = 1,
	MSG_STOP_ALARM = 2,			// won't be needed?
	MSG_ANNOUNCE_NAME = 3,
	MSG_INVALID = 0
};

struct MessageRawBase {
	uint8_t m_mac[6];			// mac address of the original sender
	uint8_t m_msgType;			// type of message
	uint8_t m_alarm;			// whether the alarm activated or not
	uint32_t m_msgId;			// sequential message id, strictly increasing(indicates whether a given message is outdated)
};

static_assert(sizeof(MessageRawBase) == 12, "sizeof(MessageRawBase) != 12");

// MessageRaw is being transmitted between ESPs
struct MessageRaw : MessageRawBase {
	static const int MAX_LEN_DATA = 2 * sizeof(float) < MAX_LEN_ESP_NAME ? MAX_LEN_ESP_NAME : 2 * sizeof(float);

	// The body of the message. 2 * (sensor values) OR (device name)
	uint8_t m_data[MAX_LEN_DATA];
};

// Use the data from MessageRaw to fill Message. 
// This struct has a bigger size, but it would be easier to work with it throughout the code.
struct Message {
	char m_mac[6];
	MessageType m_msgType;
	bool m_alarm;
	uint32_t m_msgId;

	union {
		struct {
			float temp;
			float humidity;
		};
		
		char name[MAX_LEN_ESP_NAME];
		// add more members?
	} m_data;
};

MessageRaw prepareMessageForTransmission(const Message &msg);
Message getTransmittedMessage(const MessageRaw &msg);

#endif // !_MESSAGE_LIBRARY_
