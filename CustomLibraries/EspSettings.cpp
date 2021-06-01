#include "EspSettings.h"
#include "Timer.h"
#include <esp_wifi.h>
#include <HardwareSerial.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <cstring>

const char *ESPSettings::SettingsFileName = "/settings.bin";

ESPSettings::ESPSettings()
	: m_espNowChannel(0) {
	memset(m_espName, 0, sizeof(m_espName));
	memset(m_espNetworkName, 0, sizeof(m_espNetworkName));
	memset(m_espNetworkKey, 0, sizeof(m_espNetworkKey));
}

ESPSettings::~ESPSettings() {
	SPIFFS.end();
}

ESPSettings& ESPSettings::instance() {
	static ESPSettings instance;
	return instance;
}

bool ESPSettings::setup() {
	if (!SPIFFS.begin(true)) {
		Serial.println("Error! Could not mount SPIFFS.");
		return false;
	}

	if (existSettingsFile()) {
		Serial.println("There is a file with settings in the memory.");
		loadSettingsFromFile();
	}

	return true;
}

bool ESPSettings::existSettingsFile() const {
	return SPIFFS.exists(SettingsFileName);
}

bool ESPSettings::loadSettingsFromFile() {
	File file = SPIFFS.open(SettingsFileName, FILE_READ);
	if (!file) {
		Serial.print("Couldn't open for reading: ");
		Serial.println(SettingsFileName);
		return false;
	}

	file.readBytesUntil('\0', m_espName, sizeof(m_espName));
	file.readBytesUntil('\0', m_espNetworkName, sizeof(m_espNetworkName));
	file.readBytesUntil('\0', m_espNetworkKey, sizeof(m_espNetworkKey));
	m_espNowChannel = (uint8_t)file.read();

	file.close();

	return true;
}

bool ESPSettings::saveSettingsToFile() {
	File file = SPIFFS.open(SettingsFileName, FILE_WRITE);
	if (!file) {
		Serial.print("Couldn't open for writing: ");
		Serial.println(SettingsFileName);
		return false;
	}

	file.write((const uint8_t*)m_espName, strlen(m_espName) + 1);
	file.write((const uint8_t*)m_espNetworkName, strlen(m_espNetworkName) + 1);
	file.write((const uint8_t*)m_espNetworkKey, strlen(m_espNetworkKey) + 1);
	file.write(m_espNowChannel);

	file.clearWriteError();
	file.close();

	return true;
}

bool ESPSettings::setESPName(const char *name) {
	const int len = strlen(name);
	if (len + 1 >= MAX_LEN_ESP_NAME) {
		return false;
	}

	strcpy(m_espName, name);

	return true;
}

bool ESPSettings::setESPNetworkName(const char *name) {
	const int len = strlen(name);
	if (len + 1 >= MAX_LEN_NETWORK_NAME) {
		return false;
	}

	strcpy(m_espNetworkName, name);

	return true;
}

bool ESPSettings::setESPNetworkKey(const char *name) {
	const int len = strlen(name);
	if (len + 1 >= MAX_LEN_NETWORK_KEY) {
		return false;
	}

	strcpy(m_espNetworkKey, name);

	return true;
}

bool ESPSettings::setESPNowChannel(uint32_t channel) {
	// From ESP-Now docs.
	if (channel > 14) {
		return false;
	}

	m_espNowChannel = (uint8_t)channel;

	return true;
}

bool ESPSettings::updateSettings() {
	return saveSettingsToFile();
}

bool ESPSettings::deleteSettings() {
	return SPIFFS.remove(SettingsFileName);
}

void ESPSettings::setupUserSettings() {
	WiFi.disconnect();
	WiFi.mode(WIFI_AP);

	static const char *ssid = "MasterDevice";
	static const char *password = NULL;

	bool settingsReady = false;
	const bool hasSettings = existSettingsFile() && loadSettingsFromFile();
	bool passwordReady = hasSettings;

	static const unsigned long settingsTimeout = 2 * 60 * 1000; // 2 mins

	if (false == WiFi.softAP(ssid, password, WiFi.channel()/* default channel */, true/* hidden */)) {
		Serial.println("Failed to initialize softAP. Restarting...");
		esp_restart();
	}

	IPAddress ip = WiFi.softAPIP();
	Serial.print("Server IP address: ");
	Serial.println(ip);

	static const char *index_html = R"===(
  <!DOCTYPE HTML>
  <html>
  <head>
  <title>Master ESP setup</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head>
  <body>
  <form action="/set" method="post">
    <label for="esp_name">ESP name:</label><br>
    <input type="text" if="esp_name" name="esp_name"><br>
    <label for="network_name">Network name:</label><br>
    <input type="text" if="network_name" name="network_name"><br>
    <label for="network_key">Network password:</label><br>
    <input type="text" if="network_key" name="network_key"><br><br>
    <input type="submit" value="Submit">
  </form>
  <form action="/exit" method="post">
     <input type="submit" value="Exit(stop the server)">
  </form>
  <p>You must click the exit button when you are ready!</p>
  </body>
  </html>
  )===";
	
	WebServer webServer(80);
	webServer.on("/", HTTP_GET, [&]() {
		webServer.send(200, "text/html", index_html);
	});

	webServer.onNotFound([&]() {
		webServer.send(404, "text/html", "<p>Page not found.</p><a href=\"/\">Go back</a>");
	});

	webServer.on("/set", HTTP_POST, [&]() {
		String response;

		if (webServer.hasArg("esp_name")) {
			const String &espname = webServer.arg("esp_name");

			Serial.print("ESP name: ");
			Serial.println(espname);

			if (espname.length() > 0) {
				setESPName(espname.c_str());
				response += "<p>ESP name has been updated.</p>";
			}
		}

		if (webServer.hasArg("network_name")) {
			const String &networkName = webServer.arg("network_name");

			Serial.print("Network name: ");
			Serial.println(networkName);

			if (networkName.length() > 0) {
				setESPNetworkName(networkName.c_str());
				response += "<p>Network Name has been updated.</p>";
			}
		}

		if(webServer.hasArg("network_key")) {
			const String &networkKey = webServer.arg("network_key");

			Serial.print("Network key: ");
			Serial.println(networkKey);

			if (strlen(networkKey.c_str()) >= 8) {
				setESPNetworkKey(networkKey.c_str());
				response += "<p>Network Key has been updated.</p>";
				passwordReady = true;
			}
			else {
				response += "<p>Invalid Network Key!</p>";
			}
		}

		// Save the settings to my internal memory.
		updateSettings();
		response += "<a href=\"/\">Go back</a>";

		webServer.send(200, "text/html", response);
	});

	webServer.on("/exit", HTTP_POST, [&]() {
		if (!passwordReady) {
			webServer.send(200, "text/html", "<p>The settings are not completed!</p><a href=\"/\">Go back</a>");
			return;
		}

		settingsReady = true;
		webServer.send(200, "text/plain", "Stopping the server...");
	});

	webServer.begin();
	Serial.println("Settings server started.");

	if (hasSettings) {
		Serial.print("The settings are loaded from the memory, there is a timeout of ");
		Serial.print(settingsTimeout / 1000.f);
		Serial.println(" secs.");
	}
	else {
		Serial.println("There are no settings in the memory.");
	}

	Timer beginTimeServer;

	// Run the server until the user exits manually or the timeout runs out.
	while (((!hasSettings) || (hasSettings && beginTimeServer.elapsedTime() < settingsTimeout))) {
		webServer.handleClient();

		if (settingsReady) {
			break;
		}
	}

	webServer.close();
	Serial.println("Settings server stopped.");
	
	if (!WiFi.softAPdisconnect(false) && !WiFi.enableAP(false)) {
		Serial.println("Failed to disconnect softAP!");
	}
}
