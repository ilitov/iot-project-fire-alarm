#include "LEDBlink.h"
#include <esp32-hal.h>

LEDBlink::LEDBlink(int LEDPin, int delay)
	: m_pin(LEDPin)
	, m_delay(delay)
	, m_run(ATOMIC_FLAG_INIT) {

	pinMode(2, OUTPUT); // init the LED
}

LEDBlink::~LEDBlink() {
	stop();
}

void LEDBlink::start() {
	m_run.test_and_set();

	m_thread = std::thread([this]() {
		while (m_run.test_and_set()) {
			digitalWrite(m_pin, HIGH);
			delay(m_delay);
			digitalWrite(m_pin, LOW);
			delay(m_delay);
		}
	});
}

void LEDBlink::stop() {
	m_run.clear();

	if (m_thread.joinable()) {
		m_thread.join();
	}
}
