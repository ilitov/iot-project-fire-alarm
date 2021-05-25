#include "MessagesCallbacks.h"
#include "../EspNowManagerLibrary/EspNowManager.h"

MasterCallback::MasterCallback(EspNowManager &espman)
	: m_espman(&espman) {

}

void MasterCallback::operator()(const Message &msg) {
	if (!m_espman) {
		return;
	}

	// Reset the entries in the map, which correspond to the MAC address of the sender.
	if (isAuthorizationMessage(msg.m_msgType)) {

		// TODO: Store the data in my internal "database".
		if (msg.m_msgType == MessageType::MSG_ANNOUNCE_NAME) {
			m_espman->m_mapMessages.eraseLogForMacAddress(msg.m_mac);

			// Store data(mac - name) about the new ESP...

			// Add the ESP as a peer.
			// This call can fail if we have no space for peers, but we hope that at least someone will add it as a peer
			// and the message will be delivered eventually.
			if (ESP_OK != m_espman->addPeer(msg.m_mac)) {
				Serial.println("Could not add a new peer in the master(MessageType::MSG_ANNOUNCE_NAME).");

				// TODO: Check for inactive peers and delete those of them who are inactive.
				//		 That way we will free space for new peers.
			}

			// Send MASTER_ACK message to the slaves.
			Message replyMessage;
			MessagesMap::parseMacAddress(m_espman->m_myMAC, replyMessage.m_mac);
			replyMessage.m_msgType = MessageType::MSG_MASTER_ACK;
			replyMessage.m_msgId = 0; // doesn't matter as authorization messages are not being saved in the messages map
			replyMessage.m_alarmStatus = AlarmType::ALARM_NO_SMOKE_OFF; // doesn't matter as well
				

			MessageRaw transmitMsg{};
			const int length = prepareMessageForTransmission(replyMessage, transmitMsg);
			m_espman->sendData(reinterpret_cast<const uint8_t*>(&transmitMsg), length);
		}
	}

	// TODO: Re-write this function.

	Serial.println("Sending a message to MQTT server:");
	Serial.print("Message author: ");
	
	for (int i = 0; i < LEN_ESP_MAC_ADDRESS; ++i) {
		Serial.print(msg.m_mac[i], HEX);
		Serial.print(':');
	}

	Serial.println();
	Serial.print("Message type: ");
	switch (msg.m_msgType) {
	case MessageType::MSG_ANNOUNCE_NAME:
		Serial.println("MSG_ANNOUNCE_NAME");
		break;
	case MessageType::MSG_SENSOR_DATA:
		Serial.println("MSG_SENSOR_DATA");
		break;
	case MessageType::MSG_STOP_ALARM:
		Serial.println("MSG_SENSOR_DATA");
		break;
	case MessageType::MSG_MASTER_ACK:
		Serial.println("MSG_MASTER_ACK");
		break;
	default:
		break;
	}

	Serial.print("Alarm type: ");
	switch (msg.m_alarmStatus) {
	case AlarmType::ALARM_NO_SMOKE_OFF:
		Serial.println("ALARM_NO_SMOKE_OFF");
		break;
	case AlarmType::ALARM_NO_SMOKE_ON:
		Serial.println("ALARM_NO_SMOKE_ON");
		break;
	case AlarmType::ALARM_SMOKE_OFF:
		Serial.println("ALARM_SMOKE_OFF");
		break;
	case AlarmType::ALARM_SMOKE_ON:
		Serial.println("ALARM_SMOKE_ON");
		break;
	default:
		break;
	}

	Serial.print("Message ID: ");
	Serial.println(msg.m_msgId);

	Serial.println();
}

SlaveCallback::SlaveCallback(EspNowManager &espman)
	: m_espman(&espman) {

}

void SlaveCallback::operator()(const Message &msg) {
	if (!m_espman) {
		return;
	}

	if (isAuthorizationMessage(msg.m_msgType)) {
		if (msg.m_msgType == MessageType::MSG_ANNOUNCE_NAME) {
			m_espman->m_mapMessages.eraseLogForMacAddress(msg.m_mac);

			// Add the ESP as a peer.
			if (ESP_OK != m_espman->addPeer(msg.m_mac)) {
				// TODO: Check for inactive peers and delete those of them who are inactive.
				//		 That way we will free space for new peers.
			}
		}
		// Notify the slave that the master has seen its presence.
		else if (msg.m_msgType == MessageType::MSG_MASTER_ACK) {
			m_espman->m_masterAcknowledged.store(true, std::memory_order_release);
		}
	}
	
	MessageRaw transmitMsg{};
	const int length = prepareMessageForTransmission(msg, transmitMsg);
	m_espman->sendData(reinterpret_cast<const uint8_t*>(&transmitMsg), length);
}
