#ifndef _ESP_SETTINGS_HEADER_
#define _ESP_SETTINGS_HEADER_

#include <atomic>
#include <cstdint>
#include <vector>
#include <Arduino.h>
#include "MessageLibrary.h"

// A singleton class that holds settings/data for the current ESP.
// The settings can be modified by a user through a WebServer, read and written to the disk.
class ESPSettings {
	static const char *SettingsFileName;
	static const int MAX_LEN_NETWORK_NAME = MAX_LEN_ESP_NAME;
	static const int MAX_LEN_NETWORK_KEY = 17;

public:
	static const uint8_t INVALID_CHANNEL = 42;

	~ESPSettings();

	static ESPSettings &instance();

	bool init();
	bool existSettingsFile() const;

	bool setESPName(const char *name);
	bool setESPNetworkName(const char *name);
	bool setESPNetworkKey(const char *name);
	bool setESPNowChannel(uint32_t channel);

	bool updateSettings();
	bool deleteSettings();
	void setupUserSettings(bool master);

	const char* getESPName() const { return m_espName; }
	const char* getESPNetworkName() const { return m_espNetworkName; }
	const char* getESPNetworkKey() const { return m_espNetworkKey; }
	uint8_t getESPNowChannel() const { return m_espNowChannel.load(); }

	std::vector<String>& getPeersMACAddresses() { return m_peersMACAddresses; }

private:
	ESPSettings();

	bool loadSettingsFromFile();
	bool saveSettingsToFile();

private:
	char m_espName[MAX_LEN_ESP_NAME];
	char m_espNetworkName[MAX_LEN_NETWORK_NAME];
	char m_espNetworkKey[MAX_LEN_NETWORK_KEY];
	std::atomic<uint8_t> m_espNowChannel;
	// Add more members if needed.

	std::vector<String> m_peersMACAddresses;
};

#endif // !_ESP_SETTINGS_HEADER_
