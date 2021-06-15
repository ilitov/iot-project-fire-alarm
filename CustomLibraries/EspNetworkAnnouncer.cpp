#include "EspNetworkAnnouncer.h"
#include "EspNowManager.h"
#include "Timer.h"
#include <WiFi.h>
#include <WebServer.h>

struct PeerInfo {
	String ssid;
	int32_t rssi;
	int32_t channel;
	unsigned char bssid[LEN_ESP_MAC_ADDRESS + 1];
};

ESPNetworkAnnouncer::ESPNetworkAnnouncer()
	: m_run(ATOMIC_FLAG_INIT)
	, m_server(PORT) 
	, m_announceStarted(false) {

}

ESPNetworkAnnouncer::~ESPNetworkAnnouncer() {
	m_run.clear();

	if (m_thread.joinable()) {
		m_thread.join();
	}
}

ESPNetworkAnnouncer& ESPNetworkAnnouncer::instance() {
	static ESPNetworkAnnouncer instance;
	return instance;
}

void ESPNetworkAnnouncer::initSoftAP() {
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

	if (!WiFi.mode(WIFI_AP_STA)) {
		Serial.println("Failed to change the mode to WIFI_AP_STA.");
		return;
	}

	if (false == WiFi.softAP(ssid, password, espSettings.getESPNowChannel(), 0 /* hidden */, INT_MAX)) {
		Serial.println("Failed to start the NetworkAnnouncer!");
		return;
	}

	IPAddress localIP(192, 168, 42, 42);
	while (!WiFi.softAPConfig(localIP, localIP, IPAddress(255, 255, 255, 0))) {
		delay(10);
	}

	Serial.print("softAP IP: ");
	Serial.println(WiFi.softAPIP());

	m_server.begin();
	delay(100);
	m_announceStarted = true;
}

void ESPNetworkAnnouncer::deinitSoftAP() {
	m_server.stop();
	delay(100);
	m_announceStarted = false;

	WiFi.softAPdisconnect(false);
	if (!WiFi.enableAP(false)) {
		Serial.println("Failed to stop softAP in NetworkAnnouncer!");
	}
}

void ESPNetworkAnnouncer::searchForPeers() {
	Timer searchTimer;

	while (searchTimer.elapsedTime() < SEARCH_FOR_TIME) {
		if (searchForPeersImpl()) {
			return;
		}

		delay(1000);
	}
}

bool ESPNetworkAnnouncer::searchForPeersImpl() {
	const char *ssid = ESPSettings::instance().getESPNetworkName();
	const char *password = ESPSettings::instance().getESPNetworkKey();

	Serial.println("Searching for:");
	Serial.print("SSID: ");
	Serial.println(ssid);
	Serial.print("Password: ");
	Serial.println(password);

	// We have to search on the same channel as ESP-Now. Otherwise, there are ESP-Now send failures for the time of search.
	const uint8_t searchChannel = ESPSettings::instance().getESPNowChannel() == ESPSettings::INVALID_CHANNEL ? 0 : ESPSettings::instance().getESPNowChannel();

	const unsigned int count = WiFi.scanNetworks(false, true, false, 300u, searchChannel);

	Serial.print("Found ");
	Serial.print(count);
	Serial.println(" WiFi networks.");

	const int MAX_PEERS = 10;
	PeerInfo possiblePeers[MAX_PEERS];

	int idx = 0;

	for (unsigned int i = 0; i < count && idx < MAX_PEERS; ++i) {
		PeerInfo info;
		info.ssid = WiFi.SSID(i);
		info.rssi = WiFi.RSSI(i);
		info.channel = WiFi.channel();
		memset(info.bssid, 0, sizeof(info.bssid));
		MessagesMap::parseMacAddressReadable(WiFi.BSSIDstr(i).c_str(), info.bssid);

		/*Serial.print(i + 1);
		Serial.print(": ");
		Serial.print(info.ssid);
		Serial.print(" - channel: ");
		Serial.println(info.channel);*/

		// Add peers that match our network name.
		if (0 == strcmp(info.ssid.c_str(), ESPSettings::instance().getESPNetworkName())	/*|| info.ssid.isEmpty() *//* hidden networks have empty SSID */) {
			possiblePeers[idx++] = info;
		}
	}

	// Free memory.
	WiFi.scanDelete();

	// Bigger rssi is better.
	std::sort(possiblePeers, possiblePeers + idx, [](const PeerInfo &a, const PeerInfo &b) {
		return a.rssi > b.rssi;
	});

	// Add at most 2 peers on a search.
	bool foundPeer = false;
	for (int i = 0, addedPeers = 0; i < idx && addedPeers < 3; ++i) {
		const bool currResult = processPeer(possiblePeers[i], ssid, password);
		if (currResult) {
			++addedPeers;
		}

		foundPeer = foundPeer || currResult;
	}

	return foundPeer;
}

void ESPNetworkAnnouncer::begin() {
	// If it is already started, return.
	if (m_run.test_and_set()) {
		return;
	}

	m_thread = std::thread([this]() {

		while (m_run.test_and_set()) {
			m_startAnnounce.store(true, std::memory_order_release);
			delay(SLEEP_FOR_TIME);
		}

	});
}

bool ESPNetworkAnnouncer::processPeer(PeerInfo &peer, const char *ssid, const char *password) {
	static const char *host = "192.168.42.42";
	static const int port = PORT;

	uint8_t wifiChannel = ESPSettings::INVALID_CHANNEL;
	String MACaddress;
	MACaddress.reserve(LEN_ESP_MAC_ADDRESS + 1);

	Timer timeout;
	WiFi.begin(ssid, password, peer.channel, peer.bssid);

	Serial.print("Connecting to peer: ");
	Serial.println(ssid);

	// Wait for connection(max 3 secs).
	while (WiFi.status() != WL_CONNECTED && timeout.elapsedTime() < 6000) {
		//Serial.println("Connecting to peer SoftAP...");
		delay(1000);
	}

	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("Failed to connect to the peer.");
		WiFi.disconnect();
		return false;
	}

	Serial.println("Connected to peer.");

	// Receive the data from the server.
	//HTTPClient client;
	//
	//if (!client.begin(host, port, "/")) {
	//	Serial.println("Couldn't connect to the peer's server.");
	//}
	//else {
	//	const int responseCode = client.GET();

	//	if (responseCode == HTTP_CODE_OK) {
	//		// Get message body.
	//		const String &peerMACAddress = client.getString();
	//		Serial.print("Peer MAC: ");
	//		Serial.println(peerMACAddress);
	//	}
	//	else {
	//		Serial.println("Invalid reponse(HTTP code) from peer.");
	//	}
	//}

	//client.end();

	// I'm using the simple client, because there are exceptions with HTTPClient from time to time?!
	WiFiClient client;
	client.connect(host, port, 6000);

	if (client.connected()) {
		client.print("GET / HTTP/1.1\r\nConnection: close\r\n\r\n");
		client.flush();

		timeout.reset();
		while (0 == client.available() && timeout.elapsedTime() < 4000) {
			Serial.println("Connecting to server...");
			delay(500);
		}

		while (client.available() > 0) {
			String str = client.readStringUntil('\n');

			// Empty line between the header and the body.
			if (str.length() == 1 && str[0] == '\r') {
				wifiChannel = static_cast<uint8_t>(client.parseInt());
				client.readStringUntil('\n');

				MACaddress = client.readStringUntil('\n');
				MACaddress.setCharAt(MACaddress.length() - 1, '\0');

				Serial.print("WiFi channel: ");
				Serial.println(wifiChannel);
				Serial.print("MAC address: ");
				Serial.println(MACaddress);

				break;
			}
		}
	}

	client.stop();
	WiFi.disconnect();

	// ### Connection with peer is completed, handle the results.

	ESPSettings &settings = ESPSettings::instance();

	// There was no connection.
	if (wifiChannel == ESPSettings::INVALID_CHANNEL) {
		return false;
	}
	
	if(settings.getESPNowChannel() == ESPSettings::INVALID_CHANNEL) {
		// Set a valid channel;
		settings.setESPNowChannel(wifiChannel);

		// Add peer to queue(note that ESP-Now is not init()-ed yet and we can't add the peers directly).
		settings.getPeersMACAddresses().push_back(std::move(MACaddress));

		return true;
	}

	// If there are several peers, add them to the queue.
	if (wifiChannel == settings.getESPNowChannel()) {
		const std::vector<String> &MACAddressesQueue = settings.getPeersMACAddresses();
		
		// If the current peer is not in the list yet.
		if (!std::any_of(MACAddressesQueue.cbegin(), MACAddressesQueue.cend(), [MACaddress](const String &s) { return s == MACaddress; })) {
			settings.getPeersMACAddresses().push_back(std::move(MACaddress));
			return true;
		}
	}

	// Found a peer in the same ESP network.
	// Note: This function is meant to be called during scans after the initial ESP-Now initialization.
	// However, this feature is not supported yet and most probably, it wouldn't be supported anytime soon.

	//if (wifiChannel == settings.getESPNowChannel()) {
	//	// Reuse peer's bssid.
	//	MessagesMap::parseMacAddressReadable(MACaddress.c_str(), peer.bssid);

	//	if (!EspNowManager::instance().hasPeer(peer.bssid)) {
	//		Serial.print("Adding peer: ");
	//		for (int i = 0; i < LEN_ESP_MAC_ADDRESS; ++i) Serial.print(peer.bssid[i], HEX);
	//		Serial.println();

	//		esp_err_t res = EspNowManager::instance().addPeer(peer.bssid);
	//		ESP_ERROR_CHECK_WITHOUT_ABORT(res);
	//		return res == ESP_OK;
	//	}
	//}

	return false;
}

void ESPNetworkAnnouncer::handlePeer() {
	if (!m_startAnnounce.load(std::memory_order_acquire)) {
		return;
	}

	static Timer announceTimer;

	if (!m_announceStarted) {
		initSoftAP();
		// m_announceStarted is *true* now

		Serial.println("Starting the NetworkAnnouncer.");

		announceTimer.reset();
	}

	WiFiClient client = m_server.available();
	if (client) {
		Serial.println("There is a client.");

		client.print("HTTP/1.1 200 OK\r\n");
		client.print("Content-type:text/plain\r\n");
		client.print("\r\n");

		client.print(ESPSettings::instance().getESPNowChannel());
		client.print("\r\n");
		client.print(WiFi.macAddress());
		client.print("\r\n");
		client.flush();

		delay(300);
	}

	if (announceTimer.elapsedTime() > ANNOUNCE_FOR_TIME) {
		deinitSoftAP();
		// m_announceStarted is *false* now

		Serial.println("Stopping the NetworkAnnouncer.");

		m_startAnnounce.store(false, std::memory_order_release);
	}
}
