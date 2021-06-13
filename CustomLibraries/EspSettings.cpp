#include "EspSettings.h"
#include "PeersMessagesLibrary.h"
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
	memset(m_telegramChatID, 0, sizeof(m_telegramChatID));
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
	file.readBytes(m_telegramChatID, sizeof(m_telegramChatID));

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
	file.write((const uint8_t*)m_telegramChatID, sizeof(m_telegramChatID));

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

bool ESPSettings::setTelegramChatID(const char *chatID) {
	const int len = strlen(chatID);
	if (len + 1 >= MAX_CHAT_ID) {
		return false;
	}

	strcpy(m_espNetworkKey, chatID);

	return true;
}

bool ESPSettings::setESPNowChannel(uint32_t channel) {
	// From ESP-Now docs.
	if (channel > 14 && channel != INVALID_CHANNEL) {
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

	static const unsigned long settingsTimeout = 2 * 60 * 1000; // 2 mins

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
    <input type="text" id="esp_name" name="esp_name"><br>
    <label for="network_name">Network name:</label><br>
    <input type="text" id="network_name" name="network_name"><br>
    <label for="network_key">Network password:</label><br>
    <input type="text" id="network_key" name="network_key"><br>
    <label for="chat_id">Telegram chat ID:</label><br>
    <input type="text" id="chat_id" name="chat_id"><br><br>
    <input type="submit" value="Submit">
  </form>
  <form action="/mac" method="get">
     <input type="submit" value="Add peers(MAC addresses)">
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
	+ String("<h2>Telegram chat ID: ") + String(getTelegramChatID()) + String("</h2>")

	+ String(
		"<form action = \"/mac\" method = \"get\">											\
		<input type = \"submit\" value = \"Add peers(MAC addresses)\">						\
		</form>"
	)

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

	webServer.on("/mac", HTTP_GET, [&] {
		webServer.send(200, "text/html", htmlMACAddresses());
	});

	webServer.on("/setMACPeer", HTTP_POST, [&]() {
		if (webServer.hasArg("mac_peer")) {
			String newMAC = webServer.arg("mac_peer");

			Serial.print("new MAC: ");
			Serial.println(newMAC);
			Serial.println();
			Serial.println(newMAC.length());

			// The MAC address must not be equal to the MAC address of the current ESP and it must be valid.
			if (WiFi.macAddress() != newMAC && MessagesMap::validReadableMACAddress(newMAC.c_str())) {

				Serial.println("here");

				// If the MAC address has not been added yet.
				if (!std::any_of(m_peersMACAddresses.cbegin(), m_peersMACAddresses.cend(), [newMAC](const String &s) { return s == newMAC; })) {
					m_peersMACAddresses.push_back(std::move(newMAC));
				}
			}
		}

		webServer.send(200, "text/html", htmlMACAddresses());
	});

	webServer.on("/setChannel", HTTP_POST, [&]() {
		if (webServer.hasArg("esp_channel")) {
			const uint8_t channel = (uint8_t)webServer.arg("esp_channel").toInt();

			// The settings will be updated after the client exits. Don't update(save to file) them here.
			if (setESPNowChannel(channel) /* && updateSettings() */) {
				Serial.print("In /setChannel. The channel has been updated to: ");
				Serial.println(channel);
			}
		}

		webServer.send(200, "text/html", htmlMACAddresses());
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

			if (webServer.hasArg("chat_id")) {
				const String &chatID = webServer.arg("chat_id");

				Serial.print("Telegram chat ID: ");
				Serial.println(chatID);

				if (setTelegramChatID(chatID.c_str())) {
					response += "<p>Telegram chat ID has been updated.</p>";
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

String ESPSettings::htmlMACAddresses() const {
	static const char *macAddresses_html1 = R"===(
  <!DOCTYPE HTML>
  <html>
  <head>
  <title>ESP setup</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head>
  <body>
  <h3>My MAC address:</h3>
  )===";

	static const char *macAddresses_html2 = R"===(
  <form action="/setMACPeer" method="post">
    <label for="mac_peer">MAC address of a peer:</label><br>
    <input type="text" id="mac_peer" name="mac_peer"><br>
    <input type="submit" value="Submit">
  </form><br>
  <a href="/">Go back to the main page.</a>
  </body>
  </html>
  )===";

	String result(macAddresses_html1);

	// My MAC address
	result += "<p>" + WiFi.macAddress() + "</p>";

	result += " <h3>List of added MAC addresses:</h3>";

	for (const String &s : m_peersMACAddresses) {
		result += "<p>" + s + "</p>";
	}

	result +=	"<span>You can add </span><span>" +
				String(MAX_PEERS_FROM_USER - (int)m_peersMACAddresses.size()) +
				"</span><span> more peers.</span><br>";

	result +=	"<span>The ESP is using WiFi channel: </span><span>" +
				(m_espNowChannel != INVALID_CHANNEL ? String(m_espNowChannel) : String("INVALID")) +
				"</span>";

	result += 
	R"===(
	<form action="/setChannel" method="post">
		<label for="esp_channel">Set a WiFi channel(you can leave it blank):</label>
		<input type="number" id="esp_channel" name="esp_channel"><br>
		<input type="submit" value="Submit">
	</form><hr>
	)===";

	result += macAddresses_html2;

	return result;
}
