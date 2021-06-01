#ifndef _ESP_NETWORK_ANNOUNCER_
#define _ESP_NETWORK_ANNOUNCER_

#include <thread>
#include <atomic>
#include "EspSettings.h"

class ESPNetworkAnnouncer {
public:
	static const unsigned long SLEEP_FOR_TIME = 1 * 60 * 1000; // sleep for 1 minute
	static const unsigned long WORK_FOR_TIME = 2 * 60 * 1000; // sleep for 2 minutes

	ESPNetworkAnnouncer();
	~ESPNetworkAnnouncer();

	void announce();
	void begin();

private:
	std::atomic_flag m_run;
	std::thread m_thread;
};

#endif // !_ESP_NETWORK_ANNOUNCER_
