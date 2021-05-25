#include "EspNowManager.h"

#include <cstring>

EspNowManager::EspNowManager()
	: m_peerChannel(0)
	, m_active(false)
	, m_canSendData(true)
	, m_peersMessagesProcessor(NULL)
	, m_myMessagesProcessor(NULL)
	, m_masterAcknowledged(false) {

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

bool EspNowManager::init(MessagesProcessorBase *peersmp, MessagesProcessorBase *mymp, uint8_t channel, bool isMasterESP, const char *myMAC) {
	if (m_active) {
		return false;
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

	if (peersmp == mymp) {
		Serial.println("Peers processor must be different from my messages processor! You can use the same class, but there must be 2 instances!");
		return false;
	}

	m_peerChannel = channel;
	m_active = true;
	m_myMAC = MessagesMap::parseMacAddressReadable(myMAC);
	
	// Do not wait for master if I am the master.
	if (isMasterESP) {
		m_masterAcknowledged.store(std::memory_order_release);
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

	esp_now_peer_info_t peerInfo{};
	std::memcpy(peerInfo.peer_addr, reinterpret_cast<const uint8_t*>(macAddr), ESP_NOW_ETH_ALEN);
	peerInfo.channel = m_peerChannel;
	peerInfo.encrypt = false;

	return esp_now_add_peer(&peerInfo);
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
	esp_err_t err = esp_now_get_peer(reinterpret_cast<const  uint8_t*>(macAddr), &peer);

	return err != ESP_ERR_ESPNOW_NOT_FOUND;
}

bool EspNowManager::enqueueSendDataAsync(const Message &msg) {
	if (m_myMessagesProcessor) {
		return m_myMessagesProcessor->push(msg);
	}

	return false;
}

void EspNowManager::receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLength) {
	// The size of every message must be >= sizeof(MessageRawBase)
	if (dataLength < sizeof(MessageRawBase)) {
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

	if(!isAuthorization && isNewMessage != espMan.m_mapMessages.addMessage(mac, message.m_msgType, message.m_msgId)) {
		// The slave has already seen this message. And it is not related to authorization to/from master ESP.
		return;
	}

	// Push the message to a queue(which another thread processes) in order to be retransmitted with sendData()
	// or sent to a MQTT server(master ESP).
	if (espMan.m_peersMessagesProcessor) {
		espMan.m_peersMessagesProcessor->push(message);
	}
}

void EspNowManager::sendCallback(const uint8_t *macAddr, esp_now_send_status_t status) {
	// TODO: Do something in case of a failure(log a command in a queue?).
	if (status == ESP_NOW_SEND_FAIL) {
		Serial.print("Sending data to MAC: ");
		for (int i = 0; i < ESP_NOW_ETH_ALEN; ++i) {
			Serial.print(macAddr[i], HEX);
			Serial.print(':');
		}
		Serial.println(" failed!");
	}

	EspNowManager &espMan = instance();

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
