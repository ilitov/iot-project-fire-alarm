#ifndef _ESP_NETWORK_ANNOUNCER_
#define _ESP_NETWORK_ANNOUNCER_

#include <thread>
#include <atomic>
#include "EspSettings.h"

struct PeerInfo;

class ESPNetworkAnnouncer {
public:
	static const unsigned long SLEEP_FOR_TIME = 1 * 10 * 1000; // sleep for 3 minutes
	static const unsigned long WORK_FOR_TIME = 1 * 60 * 1000; // work for 1 minute
	static const unsigned long SEARCH_FOR_TIME = 1 * 60 * 1000; // search peers for 1 minute

	static ESPNetworkAnnouncer& instance();

	~ESPNetworkAnnouncer();

	void begin();
	void notifySearchPeers();

	bool isSearching() const { return m_searchPeers.load(std::memory_order_acquire); }

private:
	ESPNetworkAnnouncer();
	
	void announce();
	void searchForPeers();
	bool processPeer(PeerInfo &peer, const char *ssid, const char *password, bool &stop);

	std::atomic_flag m_run;
	std::atomic<bool> m_searchPeers;
	std::thread m_thread;
};

#endif // !_ESP_NETWORK_ANNOUNCER_
