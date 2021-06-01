#include "Timer.h"
#include <esp32-hal.h>

Timer::Timer() {
	reset();
}

void Timer::reset() {
	m_time = millis();
}

unsigned long Timer::elapsedTime() {
	const unsigned long timeNow = millis();

	if (timeNow >= m_time) {
		return timeNow - m_time;
	}

	return timeNow + (static_cast<unsigned long>(-1) - m_time);
}
