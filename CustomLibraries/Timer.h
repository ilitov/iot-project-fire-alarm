#ifndef _MY_TIMER_HEADER_
#define _MY_TIMER_HEADER_

#include <atomic>
#include <esp32-hal.h>

class Timer {
public:
	Timer(unsigned long time = millis());
	Timer(const Timer &);
	Timer& operator=(const Timer &);
	~Timer() = default;

	void reset();
	unsigned long elapsedTime() const;
	unsigned long elapsedTime(unsigned long timeNow) const;
	unsigned long time() const { return m_time.load(); }

private:
	std::atomic<unsigned long> m_time;
};

#endif // !_MY_TIMER_HEADER_
