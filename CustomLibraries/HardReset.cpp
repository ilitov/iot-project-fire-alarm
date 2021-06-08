#include "HardReset.h"
#include "EspSettings.h"
#include <HardwareSerial.h>
#include <esp32-hal.h>

HardReset::HardReset(int FLASH_BUTTON)
	: m_button(FLASH_BUTTON) {

}

HardReset::~HardReset() {
	disable();
}

HardReset& HardReset::instance(int FLASH_BUTTON) {
	static HardReset instance(FLASH_BUTTON);
	return instance;
}

void HardReset::setup() {
	if (m_run.test_and_set()) {
		return;
	}

	pinMode(m_button, INPUT_PULLUP);
	attachInterrupt(m_button, handleInterrupt, RISING);

	m_thread = std::thread([this]() {
		while (m_run.test_and_set()) {
			kick();
			delay(HOLD_SECONDS / 4u);
		}
	});
}

void HardReset::kick() {
	// Reset the clock if the button is not down.
	if (digitalRead(m_button) != LOW) {
		m_time.reset();
	}
	else if (m_time.elapsedTime() >= HOLD_SECONDS) {
		Serial.println("Resetting...");

		// Delete the file with ESP settings.
		if (!ESPSettings::instance().deleteSettings()) {
			Serial.println("Could not delete ESP settings.");
		}

		Serial.println("Rebooting...");
		esp_restart();
	}
}

void HardReset::handleInterrupt() {
	HardReset::instance().m_time.reset();
}

void HardReset::disable() {
	m_run.clear();

	if (m_thread.joinable()) {
		m_thread.join();
	}

	m_run.clear();
}
