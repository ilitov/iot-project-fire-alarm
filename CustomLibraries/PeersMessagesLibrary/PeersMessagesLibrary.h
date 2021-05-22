#ifndef _PEERS_MESSAGES_
#define _PEERS_MESSAGES_

#include <map>
#include <cstdint>

#include "../MessageManagerLibrary/MessageLibrary.h"

class MessagesMap {
public:
	using mac_t = uint64_t; // We need 48 bits for the MAC address.
	using id_t = uint32_t;
	
	struct MessageRecord {
		bool add(const MessageType type, const id_t id);

		std::map<MessageType, id_t> m_mapTypeToId;
	};

	using container_t = std::map<mac_t, MessageRecord>;	// use std::vector instead of std::map?

public:
	// Return value:
	// true - added/updated the message -> broadcast message
	// false - the message is outdated -> ignore message
	bool addMessage(const mac_t mac, const MessageType msgType, const id_t id);
	bool addMessage(const char *mac, const MessageType msgType, const id_t id);

public:
	static mac_t parseMacAddressReadable(const char *mac);
	// We assume that the length of the mac is 6 bytes.
	static mac_t parseMacAddress(const char *mac);
	static void parseMacAddress(mac_t mac, char *destMac);

private:
	container_t m_mapMacToRecord;
};

#endif // !_PEERS_MESSAGES_
