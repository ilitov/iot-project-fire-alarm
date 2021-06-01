#include "HardReset.h"
#include "EspSettings.h"
#include <HardwareSerial.h>
#include <esp32-hal.h>

HardReset::HardReset(int FLASH_BUTTON)
	: m_button(FLASH_BUTTON) {

}

HardReset& HardReset::instance(int FLASH_BUTTON) {
	static HardReset instance(FLASH_BUTTON);
	return instance;
}

void HardReset::setup() {
	m_reset = false;
	pinMode(m_button, INPUT_PULLUP);
	attachInterrupt(m_button, handleInterrupt, RISING);
}

void HardReset::kick() {
	// Reset the clock if the button is not down.
	if (digitalRead(m_button) != LOW) {
		std::lock_guard<std::mutex> lock(m_mtx);

		if (instance().m_reset) {
			// Delete the file with ESP settings.
			if (!ESPSettings::instance().deleteSettings()) {
				Serial.println("Could not delete ESP settings.");
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
	std::lock_guard<std::mutex> lock(instance.m_mtx);

	if (instance.m_time.elapsedTime() > 3000) {		
		Serial.println("Resetting...");
		instance.m_reset = true;
	}

	instance.m_time.reset();
}
