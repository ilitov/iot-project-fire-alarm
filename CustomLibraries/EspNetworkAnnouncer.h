#ifndef _ESP_NETWORK_ANNOUNCER_
#define _ESP_NETWORK_ANNOUNCER_

#include <thread>
#include <atomic>
#include <mutex>
#include <WebServer.h>
#include "EspSettings.h"

struct PeerInfo;

class ESPNetworkAnnouncer {
	static const int PORT = 1024;

public:
	static const unsigned long SLEEP_FOR_TIME = 1 * 10 * 1000; // sleep for 3 minutes
	static const unsigned long ANNOUNCE_FOR_TIME = 1 * 60 * 1000; // work for 1 minute
	static const unsigned long SEARCH_FOR_TIME = 1 * 60 * 1000; // search peers for 1 minute

	static ESPNetworkAnnouncer& instance();

	~ESPNetworkAnnouncer();

	void begin();
	void notifySearchPeers();

	bool isSearching() const { return m_searchPeers.load(std::memory_order_acquire); }

	// Non-blocking function which must be called on each loop() iteration from the main thread.
	void handlePeer();

private:
	ESPNetworkAnnouncer();
	
	void announce();
	void searchForPeers();
	bool processPeer(PeerInfo &peer, const char *ssid, const char *password);

	std::atomic_flag m_run;
	std::atomic<bool> m_searchPeers;
	std::thread m_thread;

	WiFiServer m_server;
	bool m_announceStarted;
	std::mutex m_announceMtx;
};

#endif // !_ESP_NETWORK_ANNOUNCER_
