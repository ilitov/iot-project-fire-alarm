#include "FireAlarm.h"
#include <HardwareSerial.h> // TODO: remove

FireAlarm::FireAlarm()
	: m_isActive(false)
	, m_disableTime(0)
	, m_isDisabled(false) {

}

FireAlarm::~FireAlarm() {
	deactivate();
}

FireAlarm& FireAlarm::instance() {
	static FireAlarm instance;
	return instance;
}

void FireAlarm::begin(const int pin) {
	ledcSetup(CHANNEL, INITIAL_FREQUENCY, INITIAL_RESOLUTION);
	ledcAttachPin(pin, CHANNEL);
}

bool FireAlarm::activate() {
	if (m_isActive) {
		return true;
	}

	if (m_isDisabled) {
		if (m_disableTimer.elapsedTime() >= m_disableTime) {
			m_isDisabled = false;
			m_disableTime = 0;
		}
		else {
			return false;
		}
	}

	m_isActive = true;
	m_thread = std::thread(&FireAlarm::threadFunction, this);

	return true;
}

void FireAlarm::deactivate() {
	m_isActive = false;

	if (m_thread.joinable()) {
		m_thread.join();
	}
}

void FireAlarm::disableAlarmFor(unsigned long timeMS) {
	Serial.print("[FireAlarm] The alarm has been stopped for ");
	Serial.print(timeMS / 1000.f);
	Serial.println(" sec.");

	m_disableTime = timeMS;
	m_isDisabled = true;
	m_disableTimer.reset();
}

bool FireAlarm::alarmSet(int gasValue) {
	const int gasThreshhold = 500;
	static bool isSet = false;

	if (gasValue > gasThreshhold) {
		if (!isSet) {
			isSet = activate();
			return isSet;
		}

		return true;
	}
	else {
		isSet = false;
		deactivate();
		return false;
	}
}

void FireAlarm::threadFunction() {
	while (m_isActive && !m_isDisabled) {
		ledcWriteTone(CHANNEL, TRIGGER_FREQUENCY);
		delay(DELAY_TIME_MS);
		ledcWriteTone(CHANNEL, INITIAL_FREQUENCY);
		delay(DELAY_TIME_MS);
	}
}
