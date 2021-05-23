#ifndef _ESPNOW_MANAGER_LIBRARY_
#define _ESPNOW_MANAGER_LIBRARY_

#include <esp_now.h>
#include <HardwareSerial.h>

#include <cstdint>
#include <mutex>
#include <condition_variable>

#include "../PeersMessagesLibrary/PeersMessagesLibrary.h"
#include "../MessagesProcessor/MessagesProcessor.h"

// A singleton class that manages the ESP-Now subsystem.
class EspNowManager {
public:
	static const int MAX_PEERS = ESP_NOW_MAX_TOTAL_PEER_NUM;
	static const int MAX_PEERS_ENCRYPT = ESP_NOW_MAX_ENCRYPT_PEER_NUM;

public:
	EspNowManager(const EspNowManager &) = delete;
	EspNowManager& operator=(const EspNowManager &) = delete;
	~EspNowManager();

	static EspNowManager& instance();

	bool init(MessagesProcessorBase *peersmp, MessagesProcessorBase *mymp, uint8_t channel, const char *myMAC);

	// Note: This is a blocking call!
	esp_err_t sendData(const uint8_t *data, size_t len);
	esp_err_t addPeer(const char *macAddr);
	esp_err_t removePeer(const char *macAddr);
	bool hasPeer(const char *macAddr) const;

	// Don't call this function from multiple threads. It's NOT thread-safe!
	// Use it to enqueue messages from the current ESP.
	bool enqueueSendDataAsync(const Message &msg);

	// Both callbacks are being called from a different thread(it's same for both of them, but it's different from the main thread).
	static void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLength);
	static void sendCallback(const uint8_t *macAddr, esp_now_send_status_t status);

private:
	EspNowManager();

private:
	// We communicate with peers through a specific channel. 
	// Once we are connected to the mesh, the channel must be returned from a peer as a response.
	uint8_t m_peerChannel;

	// Whether the esp-now protocol is ready.
	bool m_active;

	bool m_iAmMaster;
	
	// Make sendData() sequential in order not to overflow the underlying protocol.
	bool m_canSendData;
	std::mutex m_sendMtx;
	std::condition_variable m_sendCV;
	
	MessagesMap m_mapMessages;

	// Process the messages that come from the peers.
	MessagesProcessorBase *m_peersMessagesProcessor;

	// Process the messages fro mthe current ESP.
	MessagesProcessorBase *m_myMessagesProcessor;

	// The MAC address of the current ESP.
	MessagesMap::mac_t m_myMAC;
};

#endif // !_ESPNOW_MANAGER_LIBRARY_
