#ifndef _MY_TIMER_HEADER_
#define _MY_TIMER_HEADER_

class Timer {
public:
	Timer();

	void reset();
	unsigned long elapsedTime();

private:
	unsigned long m_time;
};

#endif // !_MY_TIMER_HEADER_
