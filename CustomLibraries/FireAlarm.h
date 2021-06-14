#ifndef _FIRE_ALARM_HEADER_
#define _FIRE_ALARM_HEADER_

#include <thread>
#include <condition_variable>
#include <atomic>

#include "Timer.h"

class FireAlarm {
	FireAlarm();

public:
	~FireAlarm();
	static FireAlarm& instance();

	void begin(const int pin);

	bool activate();
	void deactivate();

	void disableAlarmFor(unsigned long timeMS);

	bool alarmSet(int gasValue);

private:
	void threadFunction();

	static const uint8_t CHANNEL = 0;
	static const uint8_t INITIAL_RESOLUTION = 0;
	const double INITIAL_FREQUENCY = 0;
	const double TRIGGER_FREQUENCY = 4500;
	static const int DELAY_TIME_MS = 1000;

	std::thread m_thread;
	std::atomic<bool> m_isActive;

	Timer m_disableTimer;
	std::atomic<unsigned long> m_disableTime;
	std::atomic<bool> m_isDisabled;
};

#endif // !_FIRE_ALARM_HEADER_

