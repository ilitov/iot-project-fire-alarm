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

	m_reset.store(false, std::memory_order_release);
	pinMode(m_button, INPUT_PULLUP);
	attachInterrupt(m_button, handleInterrupt, RISING);

	m_thread = std::thread([this]() {
		while (m_run.test_and_set()) {
			kick();
			delay(HOLD_SECONDS / 2);
		}
	});
}

void HardReset::kick() {
	// Reset the clock if the button is not down.
	if (digitalRead(m_button) != LOW) {

		if (instance().m_reset.load(std::memory_order_acquire)) {
			// Delete the file with ESP settings.
			if (!ESPSettings::instance().deleteSettings()) {
				Serial.println("Could not delete ESP settings.");
				instance().m_reset.store(false, std::memory_order_release);
			}
			else {
				Serial.println("Rebooting...");
				esp_restart();
			}
		}

		instance().m_time.reset();
	}
}

void HardReset::handleInterrupt() {
	HardReset &instance = HardReset::instance();

	if (instance.m_time.elapsedTime() > HOLD_SECONDS) {
		Serial.println("Resetting...");
		instance.m_reset.store(true, std::memory_order_release);
	}

	instance.m_time.reset();
}

void HardReset::disable() {
	m_run.clear();

	if (m_thread.joinable()) {
		m_thread.join();
	}

	m_run.clear();
}
