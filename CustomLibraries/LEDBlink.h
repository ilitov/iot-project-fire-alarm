#ifndef _LED_BLINK_HEADER_
#define _LED_BLINK_HEADER_

#include <thread>
#include <atomic>

class LEDBlink {
public:
	LEDBlink(int LEDPin, int delay = 500);
	~LEDBlink();

	void start();
	void stop();

private:
	int m_pin;
	int m_delay;
	std::atomic_flag m_run;
	std::thread m_thread;
};

#endif // !_LED_BLINK_HEADER_
