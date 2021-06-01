#include "EspNetworkAnnouncer.h"
#include "Timer.h"
#include <WiFi.h>
#include <WiFiServer.h>
#include <esp_wifi.h>

ESPNetworkAnnouncer::ESPNetworkAnnouncer()
	: m_run(ATOMIC_FLAG_INIT) {

}

ESPNetworkAnnouncer::~ESPNetworkAnnouncer() {
	m_run.clear();

	if (m_thread.joinable()) {
		m_thread.join();
	}
}

void ESPNetworkAnnouncer::announce() {
	if (!WiFi.mode(WIFI_AP_STA)) {
		Serial.println("Failed to change the mode to WIFI_AP_STA.");
		return;
	}

	ESPSettings &espSettings = ESPSettings::instance();
	 
	const char *ssid = *espSettings.getESPNetworkName() == '\0' ? NULL : espSettings.getESPNetworkName();
	const char *password = *espSettings.getESPNetworkKey() == '\0' ? NULL : espSettings.getESPNetworkKey();

	if (!ssid) {
		Serial.println("Invalid SSID.");
		return;
	}

	Serial.print("SSID: ");
	Serial.println(ssid);
	Serial.print("Password: ");
	Serial.println(password);

	if (false == WiFi.softAP(ssid, password, 1, true, 10)) {
		Serial.println("Failed to start the NetworkAnnouncer!");
		return;
	}

	WiFiServer server(80, 10);
	server.begin();

	Timer workTimer;

	while (workTimer.elapsedTime() < WORK_FOR_TIME) {
		WiFiClient client = server.available();
		if (client) {
			// Send the communication channel and the MAC address to the client.
			client.println(espSettings.getESPNowChannel());
			client.println(WiFi.macAddress());
			client.stop();
		}
	}

	server.stop();

	if (!WiFi.softAPdisconnect(false) && !WiFi.enableAP(false)) {
		Serial.println("Failed to stop softAPP in NetworkAnnouncer!");
	}

	WiFi.mode(WIFI_STA);
}

void ESPNetworkAnnouncer::begin() {
	m_run.test_and_set();

	m_thread = std::thread([this]() {
		while (m_run.test_and_set()) {
			Timer sleepTimer;

			do {
				delay(10000); // sleep for 10 sec
			} while (sleepTimer.elapsedTime() < SLEEP_FOR_TIME);

			Serial.println("Starting the NetworkAnnouncer.");
			announce();
			Serial.println("Stopping the NetworkAnnouncer.");
		}
	});
}
