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

	{
		std::lock_guard<std::mutex> lock(m_mtx);

		for (int i = 0; i < m_size; ++i) {
			const PeerTime &pt = m_peers[i];

			Serial.print("last time: ");
			Serial.println(pt.m_lastMsgTime.time());
			Serial.print("now time: ");
			Serial.println(time);
			Serial.print("max timeout: ");
			Serial.println(m_maxTimeout);

			if (pt.m_lastMsgTime.elapsedTime(time) > m_maxTimeout) {
				erasePeer(i);
			}
		}
	}

	for (int i = 0; i < m_size; ++i) {
		// reset() is thread-safe
		m_peers[i].m_lastMsgTime.reset();
	}
}

void PeersTimeoutWatch::updatePeer(MessagesMap::mac_t peerMAC, unsigned long time) {
	std::unique_lock<std::mutex> lock(m_mtx, std::try_to_lock);

	// Just return if the container is being updated now.
	if (!lock.owns_lock()) {
		return;
	}

	for (int i = 0; i < m_size; ++i) {
		if (m_peers[i].m_peerMAC == peerMAC) {
			m_peers[i].m_lastMsgTime.reset();
			return;
		}
	}
}

bool PeersTimeoutWatch::addPeer(MessagesMap::mac_t peerMAC, unsigned long time) {
	// This is a blocking function.
	std::lock_guard<std::mutex> lock(m_mtx);

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

bool PeersTimeoutWatch::activePeer(MessagesMap::mac_t peerMAC) const {
	std::unique_lock<std::mutex> lock(m_mtx, std::try_to_lock);

	// Just return if the container is being updated now.
	if (!lock.owns_lock()) {
		return false;
	}

	for (int i = 0; i < m_size; ++i) {
		if (m_peers[i].m_peerMAC == peerMAC && m_peers[i].m_lastMsgTime.elapsedTime() <= m_maxTimeout) {
			return true;
		}
	}

	return false;
}

int PeersTimeoutWatch::activeCount() const {
	std::unique_lock<std::mutex> lock(m_mtx, std::try_to_lock);

	// Just return if the container is being updated now.
	if (!lock.owns_lock()) {
		return -1;
	}

	int result = 0;

	for (int i = 0; i < m_size; ++i) {
		if (m_peers[i].m_lastMsgTime.elapsedTime() < m_maxTimeout) {
			++result;
		}
	}

	return result;
}
