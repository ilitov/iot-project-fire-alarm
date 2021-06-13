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
	static const unsigned long SLEEP_FOR_TIME = 1 * 20 * 1000; // sleep for 3 minutes
	static const unsigned long ANNOUNCE_FOR_TIME = 1 * 60 * 1000; // work for 1 minute
	static const unsigned long SEARCH_FOR_TIME = 1 * 60 * 1000; // search peers for 1 minute

	static ESPNetworkAnnouncer& instance();

	~ESPNetworkAnnouncer();

	void begin();
	void searchForPeers();

	// Non-blocking function which must be called on each loop() iteration from the main thread.
	void handlePeer();

private:
	ESPNetworkAnnouncer();
	
	bool searchForPeersImpl();
	bool processPeer(PeerInfo &peer, const char *ssid, const char *password);

	void initSoftAP();
	void deinitSoftAP();

	std::atomic_flag m_run;
	std::thread m_thread;

	WiFiServer m_server;
	std::atomic<bool> m_startAnnounce;
	bool m_announceStarted;
};

#endif // !_ESP_NETWORK_ANNOUNCER_
