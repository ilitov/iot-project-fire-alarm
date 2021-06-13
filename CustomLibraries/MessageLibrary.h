#ifndef _MESSAGE_LIBRARY_
#define _MESSAGE_LIBRARY_

#include <cstdint>

#define myMax(a, b) (a) < (b) ? (b) : (a)

static const int MAX_LEN_ESP_NAME = 33;
static const int LEN_ESP_MAC_ADDRESS = 6;

enum class MessageType {
	MSG_SENSOR_DATA = 0,
	MSG_STOP_ALARM = 1,
	MSG_ANNOUNCE_NAME = 2,
	MSG_MASTER_ACK = 3,
	MSG_MASTER_REQ_ACK = 4,
	MSG_INVALID = 5 // the value must be the biggest in the enum!
};

static_assert(static_cast<unsigned int>(MessageType::MSG_INVALID) < (1u << (sizeof(uint8_t) * 8u)), 
			  "There is not enough space in MessageRawBase for the values of MessageType!");

enum class AlarmType {
	ALARM_NO_SMOKE_OFF = 0,
	ALARM_NO_SMOKE_ON = 1,
	ALARM_SMOKE_OFF = 2,
	ALARM_SMOKE_ON = 3,
	ALARM_INVALID = 4 // the value must be the biggest in the enum!
};

static_assert(static_cast<unsigned int>(AlarmType::ALARM_INVALID) < (1u << (sizeof(uint8_t) * 8u)),
	"There is not enough space in MessageRawBase for the values of MessageType!");

struct MessageRawBase {
	uint8_t m_mac[6];			// mac address of the original sender
	uint8_t m_msgType;			// type of message
	uint8_t m_alarmStatus;		// whether the alarm activated or not
	uint32_t m_msgId;			// sequential message id, strictly increasing(indicates whether a given message is outdated)
};

static_assert(sizeof(MessageRawBase) == 12, "sizeof(MessageRawBase) != 12");

#pragma pack(push, 1)

// MessageRaw is being transmitted between ESPs
struct MessageRaw : MessageRawBase {
	static const int MAX_LEN_DATA = myMax(myMax(2 * sizeof(float), MAX_LEN_ESP_NAME), LEN_ESP_MAC_ADDRESS);

	// The body of the message. 2 * (sensor values) OR (device name) OR (destionation MAC address)
	uint8_t m_data[MAX_LEN_DATA];
};

#pragma pack(pop)

// Use the data from MessageRaw to fill Message. 
// This struct has a bigger size, but it would be easier to work with it throughout the code.
struct Message {
	unsigned char m_mac[6];	// MAC address of the original author
	MessageType m_msgType;	// type of message
	AlarmType m_alarmStatus;// alarm status
	uint32_t m_msgId;		// message sequential ID

	union {
		struct {
			float temp;
			float humidity;
		};
		
		char name[MAX_LEN_ESP_NAME];
		unsigned char macAddress[LEN_ESP_MAC_ADDRESS];
		struct {
			unsigned char macAddress[LEN_ESP_MAC_ADDRESS];
			unsigned long stopDurationMS;
		} stopInformation;
		// add more members?
	} m_data;				// message body(additional data)
};

// Returns the length of the message.
int prepareMessageForTransmission(const Message &msg, MessageRaw &result);
Message getTransmittedMessage(const MessageRaw &msg);

bool isAuthorizationMessage(MessageType type);

#endif // !_MESSAGE_LIBRARY_
