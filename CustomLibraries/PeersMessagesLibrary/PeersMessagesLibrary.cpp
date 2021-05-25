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

bool MessagesMap::addMessage(const unsigned char *mac, const MessageType msgType, const id_t id) {
	const mac_t iMac = mac ? parseMacAddress(mac) : 0;

	if (iMac == 0) {
		return false;
	}

	return addMessage(iMac, msgType, id);
}

bool MessagesMap::eraseLogForMacAddress(const mac_t mac) {
	return 0 != m_mapMacToRecord.erase(mac);
}

bool MessagesMap::eraseLogForMacAddress(const unsigned char *mac) {
	const mac_t iMac = mac ? parseMacAddress(mac) : 0;
	return eraseLogForMacAddress(iMac);
}

MessagesMap::mac_t MessagesMap::parseMacAddressReadable(const char *mac) {
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

void MessagesMap::parseMacAddressReadable(const char *mac, unsigned char *destMac) {
	const mac_t result = parseMacAddressReadable(mac);
	parseMacAddress(result, destMac);
}

MessagesMap::mac_t MessagesMap::parseMacAddress(const unsigned char *mac) {
	const int MAC_SIZE = 6;
	mac_t result = 0;

	// We expect that mac has a form of char[6]
	for (int i = 0; i < MAC_SIZE; ++i) {
		const uint8_t num = static_cast<uint8_t>(mac[i]);
		result <<= 8u;
		result = (result | num);
	}

	return result;
}

void MessagesMap::parseMacAddress(mac_t mac, unsigned char *destMac) {
	const int MAC_SIZE = 6;

	for (int i = MAC_SIZE - 1; i >= 0; --i) {
		destMac[i] = (mac & 0xff);
		mac >>= 8u;
	}
}
