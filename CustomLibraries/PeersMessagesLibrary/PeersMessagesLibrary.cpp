#include "PeersMessagesLibrary.h"

bool MessagesMap::MessageRecord::add(const MessageType type, const id_t id) {
	auto it = m_mapTypeToId.find(type);
	
	if (it == m_mapTypeToId.end()) {
		m_mapTypeToId.insert(std::make_pair(type, id));
		return true;
	}

	if (it->second < id) {
		it->second = id;
		return true;
	}

	return false;
}

bool MessagesMap::addMessage(const mac_t mac, const MessageType msgType, const id_t id) {
	auto it = m_mapMacToRecord.find(mac);

	if (it == m_mapMacToRecord.end()) {
		MessageRecord record;
		record.add(msgType, id);

		m_mapMacToRecord.insert(std::make_pair(mac, record));
		return true;
	}

	const bool updatedRecord = it->second.add(msgType, id);
	return updatedRecord;
}

bool MessagesMap::addMessage(const char *mac, const MessageType msgType, const id_t id) {
	mac_t iMac = mac ? parseMacAddress(mac) : 0;

	if (iMac == 0) {
		return false;
	}

	return addMessage(iMac, msgType, id);
}

MessagesMap::mac_t MessagesMap::parseMacAddress(const char *mac) {
	const int MAC_SIZE = 6;
	int readHalfBytes = 0;	// readHalfBytes * 4 bits = total number of bits
	mac_t result = 0;

	// We expect that mac has a form of ff:ff:ff:ff:ff:ff
	while (*mac != '\0' && readHalfBytes <= 2 * MAC_SIZE) {
		if (*mac != ':') {
			const uint8_t num = [](char c) {
				if (c >= 'A')  return c - 'A' + 10; // 'A' means 1010b(10 as decimal)
				return c - '0';
			}(*mac) & 0xff;

			result <<= 4u; //shift 4 bits to the left
			result = (result | num);
			++readHalfBytes;
		}

		++mac;
	}
	
	return readHalfBytes == 2 * MAC_SIZE ? result : 0;
}
