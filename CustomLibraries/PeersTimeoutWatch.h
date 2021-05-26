#ifndef _PEERS_TIMEOUT_WATCH_
#define _PEERS_TIMEOUT_WATCH_

#include <array>
#include <esp_now.h>
#include "PeersMessagesLibrary.h"

class EspNowManager;

class PeersTimeoutWatch {
	struct PeerTime {
		MessagesMap::mac_t m_peerMAC;
		unsigned long m_lastMsgTime;
	};

public:
	PeersTimeoutWatch(unsigned long maxTimeout);
	PeersTimeoutWatch(const PeersTimeoutWatch &) = default;
	PeersTimeoutWatch& operator=(const PeersTimeoutWatch &) = default;
	~PeersTimeoutWatch() = default;

	void cleanPeers(EspNowManager &espman, unsigned long time);
	void updatePeer(MessagesMap::mac_t peerMAC, unsigned long time);
	bool addPeer(MessagesMap::mac_t peerMAC, unsigned long time);

private:
	int m_size;
	const unsigned long m_maxTimeout;
	std::array<PeerTime, ESP_NOW_MAX_TOTAL_PEER_NUM> m_peers;
};

#endif // !_PEERS_TIMEOUT_WATCH_
