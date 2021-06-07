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
	: m_espNowChannel(INVALID_CHANNEL) {
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

bool ESPSettings::init() {
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

	file.readBytes(m_espName, sizeof(m_espName));
	file.readBytes(m_espNetworkName, sizeof(m_espNetworkName));
	file.readBytes(m_espNetworkKey, sizeof(m_espNetworkKey));

	uint8_t channel;
	file.readBytes((char*)&channel, sizeof(uint8_t));

	m_espNowChannel.store(channel);

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

	file.write((const uint8_t*)m_espName, sizeof(m_espName));
	file.write((const uint8_t*)m_espNetworkName, sizeof(m_espNetworkName));
	file.write((const uint8_t*)m_espNetworkKey, sizeof(m_espNetworkKey));
	file.write(m_espNowChannel.load());

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

	m_espNowChannel.store((uint8_t)channel);

	return true;
}

bool ESPSettings::updateSettings() {
	return saveSettingsToFile();
}

bool ESPSettings::deleteSettings() {
	return SPIFFS.remove(SettingsFileName);
}

void ESPSettings::setupUserSettings(bool master) {
	static const char *ssid = master ? "MasterDevice" : "SlaveDevice";
	static const char *password = NULL;

	std::atomic<bool> settingsReady{ false };
	const bool hasSettings = existSettingsFile() && loadSettingsFromFile();
	bool passwordReady = hasSettings;

	static const unsigned long settingsTimeout = 1 * 1 * 1000; // 2 mins

	WiFi.disconnect();
	WiFi.mode(WIFI_AP_STA);

	// SoftAP settings
	if (false == WiFi.softAP(ssid, password, WiFi.channel()/* default channel */, true/* hidden */)) {
		Serial.println("Failed to initialize softAP. Restarting...");
		esp_restart();
	}

	IPAddress ip = WiFi.softAPIP();
	Serial.print("Server IP address: ");
	Serial.println(ip);
	// !SoftAP settings

	static const char *index_html = R"===(
  <!DOCTYPE HTML>
  <html>
  <head>
  <title>ESP setup</title>
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

	const String index_valid_settings_html = String(
	"<!DOCTYPE HTML><html><head><title>ESP current settings</title>							\
	<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><body>")
	
	+ String("<h2>ESP Name: ") + String(getESPName()) + String("</h2>")
	+ String("<h2>ESP-Now network channel: ") + String(getESPNowChannel()) + String("</h2>")
	+ String("<h2>ESP MAC address: ") + String(WiFi.macAddress()) + String("</h2>")

	+ String(
	"<form action = \"/exit\" method = \"post\">											\
	<input type = \"submit\" value = \"Exit(stop the server)\">								\
	</form>																					\
	</body></html>");
	
	WebServer webServer(80);
	webServer.on("/", HTTP_GET, [&]() {
		webServer.send(200, "text/html", hasSettings ? index_valid_settings_html.c_str() : index_html);
	});

	webServer.onNotFound([&]() {
		webServer.send(404, "text/html", "<p>Page not found.</p><a href=\"/\">Go back</a>");
	});

	// If there are no settings in main memory, read them from the user.
	// To reset the settings, hold BOOT button for 3 seconds.
	if (!hasSettings) {
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

			if (webServer.hasArg("network_key")) {
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
	}

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
	while (!hasSettings || (hasSettings && beginTimeServer.elapsedTime() < settingsTimeout)) {
		webServer.handleClient();
		delay(200);

		// Stop the server if the user has notified us.
		if (settingsReady) {
			break;
		}
	}

	webServer.close();
	Serial.println("Settings server stopped.");
	
	WiFi.softAPdisconnect(false);
	if (!WiFi.enableAP(false)) {
		Serial.println("Failed to disconnect softAP!");
	}
}
