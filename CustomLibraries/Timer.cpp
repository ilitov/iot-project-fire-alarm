#include "Timer.h"
#include <esp32-hal.h>

Timer::Timer(unsigned long time)
	: m_time(time) {
	 
}

Timer::Timer(const Timer &r)
	: m_time(r.m_time.load()) {

}

Timer& Timer::operator=(const Timer &rhs) {
	if (this != &rhs) {
		m_time.store(rhs.m_time.load());
	}

	return *this;
}

void Timer::reset() {
	m_time.store(millis());
}

unsigned long Timer::elapsedTime() const {
	return elapsedTime(millis());
}

unsigned long Timer::elapsedTime(const unsigned long timeNow) const {
	const unsigned long time = m_time.load();

	if (timeNow >= time) {
		return timeNow - time;
	}

	return timeNow + (static_cast<unsigned long>(-1) - time);
}
