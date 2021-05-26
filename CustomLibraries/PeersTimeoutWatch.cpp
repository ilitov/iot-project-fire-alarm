#include "PeersTimeoutWatch.h"
#include "EspNowManager.h"

PeersTimeoutWatch::PeersTimeoutWatch(unsigned long maxTimeout)
	: m_size(0)
	, m_maxTimeout(maxTimeout) {

}

void PeersTimeoutWatch::cleanPeers(EspNowManager &espman, unsigned long time) {
	const auto erasePeer = [&](int &i) {
		unsigned char buff[LEN_ESP_MAC_ADDRESS];
		MessagesMap::parseMacAddress(m_peers[i].m_peerMAC, buff);
		
		espman.removePeer(buff);

		std::swap(m_peers[i], m_peers.back());
		--m_size;
		--i;
	};

	for (int i = 0; i < m_size; ++i) {
		const PeerTime &pt = m_peers[i];

		Serial.print("last time: ");
		Serial.println(pt.m_lastMsgTime);
		Serial.print("now time: ");
		Serial.println(time);
		Serial.print("max timeout: ");
		Serial.println(m_maxTimeout);

		if (time > pt.m_lastMsgTime && time - pt.m_lastMsgTime > m_maxTimeout) {
			erasePeer(i);
		}
		else if (time < pt.m_lastMsgTime) {
			const unsigned long diff = (time - 0) + (static_cast<unsigned long>(-1) - pt.m_lastMsgTime);

			if (diff > m_maxTimeout) {
				erasePeer(i);
			}
		}
	}

	for (int i = 0; i < m_size; ++i) {
		m_peers[i].m_lastMsgTime = time;
	}
}

void PeersTimeoutWatch::updatePeer(MessagesMap::mac_t peerMAC, unsigned long time) {
	for (int i = 0; i < m_size; ++i) {
		if (m_peers[i].m_peerMAC == peerMAC) {
			m_peers[i].m_lastMsgTime = time;
			return;
		}
	}
}

bool PeersTimeoutWatch::addPeer(MessagesMap::mac_t peerMAC, unsigned long time) {
	for (int i = 0; i < m_size; ++i) {
		if (m_peers[i].m_peerMAC == peerMAC) {
			return false;
		}
	}

	if (m_size < m_peers.max_size()) {
		m_peers[m_size++] = PeerTime{ peerMAC, time };
		return true;
	}

	return false;
}
