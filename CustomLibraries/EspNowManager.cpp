#include "EspNowManager.h"
#include "EspNetworkAnnouncer.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <cstring>

EspNowManager::EspNowManager()
	: m_peerChannel(0)
	, m_active(false)
	, m_isMaster(false)
	, m_canSendData(true)
	, m_peersMessagesProcessor(NULL)
	, m_myMessagesProcessor(NULL)
	, m_masterAcknowledged(false)
	, m_peersTimeoutWatch(2 * 60 * 1000 /*2 mins*/) {

}

EspNowManager::~EspNowManager() {
	if (ESP_OK != esp_now_deinit()) {
		Serial.println("Error de-initializing ESP-NOW. Stop Wi-Fi after de-initializing ESP-NOW.");
	}

	m_active = false;

	if (m_peersMessagesProcessor) {
		m_peersMessagesProcessor->terminate();
		m_peersMessagesProcessor = NULL;
	}

	if (m_myMessagesProcessor) {
		m_myMessagesProcessor->terminate();
		m_myMessagesProcessor = NULL;
	}
}

EspNowManager& EspNowManager::instance() {
	static EspNowManager espNowInstance;
	return espNowInstance;
}

bool EspNowManager::init(MessagesProcessorBase *peersmp, MessagesProcessorBase *mymp, bool isMasterESP, const char *myMAC) {
	if (m_active) {
		return false;
	}

	// Set up the communication channel.
	m_peerChannel = isMasterESP ? ESPSettings::instance().getESPNowChannel() : prepareChannel();

	if (peersmp == mymp) {
		Serial.println("Peers processor must be different from my messages processor! You can use the same class, but there must be 2 instances!");
		return false;
	}

	m_active = true;
	m_myMAC = MessagesMap::parseMacAddressReadable(myMAC);
	
	// Do not wait for master if I am the master.
	if (isMasterESP) {
		m_masterAcknowledged.store(true, std::memory_order_release);
		m_isMaster = true;
	}
	
	m_peersMessagesProcessor = peersmp;
	if (m_peersMessagesProcessor) {

		// TODO: Update the queues from the main thread at a given interval.
		if (!m_messageTasks.push(m_peersMessagesProcessor)) {
			Serial.println("Could not start the task which processes peer messages!");
		}
	}

	m_myMessagesProcessor = mymp;
	if (m_myMessagesProcessor) {

		// TODO: Update the queues from the main thread at a given interval.
		if (!m_messageTasks.push(m_myMessagesProcessor)) {
			Serial.println("Could not start the task which processes my messages!");
		}
	}

	if (ESP_OK != esp_now_init()) {
		Serial.println("Error initializing ESP-NOW. Start Wi-Fi before initializing ESP-NOW.");
		return false;
	}

	if (ESP_OK != esp_now_register_recv_cb(receiveCallback)) {
		Serial.println("Error registering ESP-NOW receive callback.");
		return false;
	}

	if (ESP_OK != esp_now_register_send_cb(sendCallback)) {
		Serial.println("Error registering ESP-NOW send callback.");
		return false;
	}

	// Add all cached peers from ESPSettings. They have been added from the user through SoftAPP configuration
	// or during the first-time prepareChannel() search for peers.
	ESPSettings &settings = ESPSettings::instance();
	std::vector<String> &peersMACs = settings.getPeersMACAddresses();

	for (const String &MAC : peersMACs) {
		unsigned char buffer[LEN_ESP_MAC_ADDRESS + 1];
		buffer[LEN_ESP_MAC_ADDRESS] = '\0';
		MessagesMap::parseMacAddressReadable(MAC.c_str(), buffer);

		addPeer(buffer);
	}

	peersMACs.clear();

	return true;
}

esp_err_t EspNowManager::sendData(const uint8_t *data, size_t len) {
	// Wait until the current thread can send data.
	std::unique_lock<std::mutex> lock(m_sendMtx);
	m_sendCV.wait(lock, [this]() { return m_canSendData; });

	// Disable sending(we hold the lock here).
	m_canSendData = false;
	
	// NULL means broadcast to all registered peers.
	esp_err_t err = m_active ? esp_now_send(NULL, data, len) : ESP_ERR_ESPNOW_NOT_INIT;

	if (err != ESP_OK) {
		// Reset the flag(sendCallback will reset it on success) and unlock.
		m_canSendData = true;
		lock.unlock();

		Serial.print("Sending data failed with: ");
		Serial.println(esp_err_to_name(err));
	}

	return err;
}

esp_err_t EspNowManager::addPeer(const unsigned char *macAddr) {
	if (!m_active) {
		return ESP_ERR_ESPNOW_NOT_INIT;
	}

	// Check if we can free some space.
	m_peersTimeoutWatch.cleanPeers(*this, millis());

	const MessagesMap::mac_t peerMAC = MessagesMap::parseMacAddress(macAddr);

	// The peer is registered and is active.
	if (m_peersTimeoutWatch.activePeer(peerMAC)) {
		return ESP_OK;
	}

	esp_now_peer_info_t peerInfo{};
	std::memcpy(peerInfo.peer_addr, reinterpret_cast<const uint8_t*>(macAddr), ESP_NOW_ETH_ALEN);
	peerInfo.channel = m_peerChannel;
	peerInfo.encrypt = false;

	esp_err_t err = esp_now_add_peer(&peerInfo);
	if (err == ESP_OK) {
		m_peersTimeoutWatch.addPeer(peerMAC, millis());
	}

	return err;
}

esp_err_t EspNowManager::removePeer(const unsigned char *macAddr) {
	if (!m_active) {
		return ESP_ERR_ESPNOW_NOT_INIT;
	}

	// We could delete peers if they have not been active for a given time.
	return esp_now_del_peer(reinterpret_cast<const uint8_t*>(macAddr));
}

bool EspNowManager::hasPeer(const unsigned char *macAddr) const {
	if (!m_active) {
		return false;
	}

	esp_now_peer_info_t peer{};
	esp_err_t err = esp_now_get_peer(reinterpret_cast<const uint8_t*>(macAddr), &peer);

	if (ESP_OK == err) {
		return m_peersTimeoutWatch.activePeer(MessagesMap::parseMacAddress(macAddr));
	}

	return err == ESP_OK;
}

bool EspNowManager::enqueueSendDataAsync(const Message &msg) {
	// Don't send non-authorization messages if the master ESP does not know about the current ESP.
	if (!isAuthorizationMessage(msg.m_msgType) && !isMasterAcknowledged()) {
		return false;
	}
	
	if (m_myMessagesProcessor) {
		return m_myMessagesProcessor->push(msg);
	}

	return false;
}

void EspNowManager::receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLength) {
	// The size of every message must be >= sizeof(MessageRawBase), but <= sizeof(MessageRaw)
	if (dataLength < sizeof(MessageRawBase) || dataLength > sizeof(MessageRaw)) {
		// Do nothing, the message is invalid.
		return;
	}

	MessageRaw receivedMessage;
	std::memcpy(&receivedMessage, data, dataLength);

	const Message &message = getTransmittedMessage(receivedMessage);

	// The current ESP is a slave.
	const MessagesMap::mac_t mac = MessagesMap::parseMacAddress(message.m_mac);


	EspNowManager &espMan = instance();

	// If the current ESP is the original sender, then drop the message.
	if (espMan.m_myMAC == mac) {
		return;
	}

	const bool isAuthorization = isAuthorizationMessage(message.m_msgType);
	const bool isNewMessage = true;

	if (!isAuthorization) {
		if (isNewMessage != espMan.m_mapMessages.addMessage(mac, message.m_msgType, message.m_msgId)) {
			// The slave has already seen this message. And it is not related to authorization to/from master ESP.
			return;
		}

		// Update the timeout of the given peer.
		espMan.m_peersTimeoutWatch.updatePeer(mac, millis());
	}

	// Push the message to a queue(which another thread processes) in order to be retransmitted with sendData()
	// or sent to a MQTT server(master ESP).
	if (espMan.m_peersMessagesProcessor) {
		espMan.m_peersMessagesProcessor->push(message);
	}
}

void EspNowManager::sendCallback(const uint8_t *macAddr, esp_now_send_status_t status) {
	EspNowManager &espMan = instance();
	
	// TODO: Do something in case of a failure(log a command in a queue?).
	if (status == ESP_NOW_SEND_FAIL) {
		Serial.print("Sending data to MAC: ");
		for (int i = 0; i < ESP_NOW_ETH_ALEN; ++i) {
			Serial.print(macAddr[i], HEX);
			Serial.print(':');
		}
		Serial.println(" failed!");
	}
	/*else { // For debug purposes.
		Serial.print("Sending data to MAC: ");
		for (int i = 0; i < ESP_NOW_ETH_ALEN; ++i) {
			Serial.print(macAddr[i], HEX);
			Serial.print(':');
		}
		Serial.println(" succeeded!");
	}*/
	
	// Update the peer if the message has been sent successfully.
	else {
		espMan.m_peersTimeoutWatch.updatePeer(MessagesMap::parseMacAddress(macAddr), millis());
	}

	// Unlock sendData() function for sending.
	{
		std::lock_guard<std::mutex> lock(espMan.m_sendMtx);
		espMan.m_canSendData = true;
	}

	espMan.m_sendCV.notify_one();
}

bool EspNowManager::isMasterAcknowledged() const {
	return m_masterAcknowledged.load(std::memory_order_acquire);
}

uint8_t EspNowManager::prepareChannel() {
	Serial.println("In prepareChannel()");

	ESPSettings &settings = ESPSettings::instance();

	// There is a channel(from the user), but there are no added peers.
	if (settings.getESPNowChannel() != ESPSettings::INVALID_CHANNEL &&
		settings.getPeersMACAddresses().empty()) {

		settings.setESPNowChannel(ESPSettings::INVALID_CHANNEL);
		settings.updateSettings();
	}

	if (settings.getESPNowChannel() == ESPSettings::INVALID_CHANNEL) {
		ESPNetworkAnnouncer::instance().searchForPeers();
	}

	const uint8_t wifiChannel = settings.getESPNowChannel();
	
	if (wifiChannel == ESPSettings::INVALID_CHANNEL) {
		Serial.println("Failed to find a valid ESP-NOW channel. Restarting...");
		esp_restart();
	}

	Serial.print("Init with WiFiChannel: ");
	Serial.println(wifiChannel);

	// Else set up the channel.
	if (ESP_OK != esp_wifi_set_channel(wifiChannel, WIFI_SECOND_CHAN_NONE)) {
		Serial.println("Failed to set up the ESP-NOW channel. Restarting...");
		esp_restart();
	}

	// The channel has been changed successfully, save it to memory.
	//ESPSettings::instance().updateSettings();

	return wifiChannel;
}

void EspNowManager::requestMasterAcknowledgement(const char *name) {
	const int length = strnlen(name, MAX_LEN_ESP_NAME);
	
	const unsigned long timeout = 60 * 1000; // 60 sec
	const unsigned int delayms = 1000;

	Timer waitTimer;

	while (!isMasterAcknowledged() && waitTimer.elapsedTime() < timeout) {
		requestMasterAcknowledgementSend(name, length);

		Serial.print("Delay ");
		Serial.print(delayms / 1000.f);
		Serial.println(" sec.");
		delay(delayms);
	}

	if (!isMasterAcknowledged()) {
		// This channel might not work at all, reset it to invalid state.
		// The user will have to enter it manually again or the	ESP will try to find it through its
		// search for peers.
		ESPSettings::instance().setESPNowChannel(ESPSettings::INVALID_CHANNEL);
		ESPSettings::instance().updateSettings();

		Serial.println("Master doesn't know about me... :(");
		esp_restart();
	}
}

void EspNowManager::requestMasterAcknowledgementSend(const char *name, const int length) {
	Message msg{};
	MessagesMap::parseMacAddressReadable(WiFi.macAddress().c_str(), msg.m_mac);
	memcpy(msg.m_data.name, name, length);
	msg.m_msgId = 0;
	msg.m_msgType = MessageType::MSG_ANNOUNCE_NAME;

	Serial.println("Connecting to master...");
	enqueueSendDataAsync(msg);
}

void EspNowManager::update() {
	// The master does not search for peers! The slaves must find the master.
	if (m_isMaster) {
		return;
	}

	// This is a non-blocking function(important! because it is being called from loop()).
	const int activePeers = m_peersTimeoutWatch.activeCount();

	// Failed to count the peers, return.
	if (activePeers == -1) {
		return;
	}

	ESPSettings &espSettings = ESPSettings::instance();

	// Very, very bad... :(((
	if (activePeers == 0) {
		Serial.println("There are no peers. Restarting the ESP...");

		// Reset the value of the channel.
		espSettings.setESPNowChannel(ESPSettings::INVALID_CHANNEL);

		esp_restart();
	}

	// Check whether the master ESP knows about the current ESP.
	const int MAX_TRIES = 5;
	static int leftTries = MAX_TRIES;

	if (!isMasterAcknowledged()) {
		const char *espName = espSettings.getESPName();
		
		requestMasterAcknowledgementSend(espName, strnlen(espName, MAX_LEN_ESP_NAME));

		delay(500);

		if (!isMasterAcknowledged()) {
			--leftTries;

			if (leftTries == 0) {
				espSettings.setESPNowChannel(ESPSettings::INVALID_CHANNEL);
				espSettings.updateSettings();

				Serial.println("Master doesn't know about me... :(");
				esp_restart();
			}
		}
	}
	else {
		leftTries = MAX_TRIES;
	}

	// Search for more peers.
	if (activePeers <= 1) {
		// TODO: Scanning for peers results in failures in ESP-Now send callback while we are waiting for a connection.
		// We might have to synchronize searchPeers()'s scan and sendData() functions.
		//ESPNetworkAnnouncer::instance().notifySearchPeers();
	}
}
