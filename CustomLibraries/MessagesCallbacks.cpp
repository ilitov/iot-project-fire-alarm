#include "MessagesCallbacks.h"
#include "EspNowManager.h"

MasterCallbackPeers::MasterCallbackPeers(EspNowManager &espman)
	: m_espman(&espman) {

}

void MasterCallbackPeers::operator()(const Message &msg) {
	if (!m_espman) {
		return;
	}

	// Reset the entries in the map, which correspond to the MAC address of the sender.
	if (isAuthorizationMessage(msg.m_msgType)) {

		// TODO: Store the data in my internal "database".
		if (msg.m_msgType == MessageType::MSG_ANNOUNCE_NAME) {
			m_espman->m_mapMessages.eraseLogForMacAddress(msg.m_mac);

			// Add the message author to the map of (MAC adress, ESP name) pairs.
			m_espman->m_macToSlaveMap[MessagesMap::parseMacAddress(msg.m_mac)] = msg.m_data.name;
			m_espman->m_slavesToMacMap[msg.m_data.name] = MessagesMap::parseMacAddress(msg.m_mac);

			Serial.print("[MessageType::MSG_ANNOUNCE_NAME] - Added a peer with ESP name: ");
			Serial.println(msg.m_data.name);

			// Add the ESP as a peer.
			// This call can fail if we have no space for peers, but we hope that at least someone will add it as a peer
			// and the message will be delivered eventually.
			// Note: addPeer() checks for inactive peers and deletes those of them which are inactive.
			if (ESP_OK != m_espman->addPeer(msg.m_mac)) {
				Serial.println("Could not add a new peer in the master(MessageType::MSG_ANNOUNCE_NAME).");

			}

			// Send MASTER_ACK message to the slave.
			Message replyMessage;
			MessagesMap::parseMacAddress(m_espman->m_myMAC, replyMessage.m_mac);
			replyMessage.m_msgType = MessageType::MSG_MASTER_ACK;
			replyMessage.m_msgId = 0; // doesn't matter as authorization messages are not being saved in the messages map
			replyMessage.m_alarmStatus = AlarmType::ALARM_NO_SMOKE_OFF; // doesn't matter as well
			memcpy(replyMessage.m_data.macAddress, msg.m_mac, sizeof(msg.m_mac)); // The MAC address of the receiver ESP.

			MessageRaw transmitMsg{};
			const int length = prepareMessageForTransmission(replyMessage, transmitMsg);
			m_espman->sendData(reinterpret_cast<const uint8_t*>(&transmitMsg), length);
		}
		else {
			Serial.print(__FILE__);
			Serial.print(' ');
			Serial.print(__LINE__);
			Serial.print(' ');
			Serial.println("This should never hepen with a single master ESP!");
		}

		return;
	}
	// It is not an authorization message, but we don't have any records for this slave ESP.
	else if (m_espman->m_macToSlaveMap.find(MessagesMap::parseMacAddress(msg.m_mac)) == m_espman->m_macToSlaveMap.end()) {
		if (ESP_OK != m_espman->addPeer(msg.m_mac)) {
			Serial.println("Could not add a new peer in the master.");
		}

		Message replyMessage;
		MessagesMap::parseMacAddress(m_espman->m_myMAC, replyMessage.m_mac);
		replyMessage.m_msgType = MessageType::MSG_MASTER_REQ_ACK;
		replyMessage.m_msgId = 0; // doesn't matter as authorization messages are not being saved in the messages map
		replyMessage.m_alarmStatus = AlarmType::ALARM_NO_SMOKE_OFF; // doesn't matter as well
		memcpy(replyMessage.m_data.macAddress, msg.m_mac, sizeof(msg.m_mac)); // The MAC address of the receiver ESP.

		MessageRaw transmitMsg{};
		const int length = prepareMessageForTransmission(replyMessage, transmitMsg);
		m_espman->sendData(reinterpret_cast<const uint8_t*>(&transmitMsg), length);

		return;
	}

	return;
	// TODO: Re-write this function to do something meaningful.

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

SlaveCallbackPeers::SlaveCallbackPeers(EspNowManager &espman)
	: m_espman(&espman) {
		
}

void SlaveCallbackPeers::operator()(const Message &msg) {
	if (!m_espman) {
		return;
	}

	if (isAuthorizationMessage(msg.m_msgType)) {

		// There is a new ESP in the mesh.
		if (msg.m_msgType == MessageType::MSG_ANNOUNCE_NAME) {
			// Clean any data related to it in my message mapping.
			m_espman->m_mapMessages.eraseLogForMacAddress(msg.m_mac);

			// Add it to my list of peers.
			m_espman->addPeer(msg.m_mac);
		}
		else if (msg.m_msgType == MessageType::MSG_MASTER_ACK && m_espman->m_myMAC == MessagesMap::parseMacAddress(msg.m_data.macAddress)) {
			// Notify the slave that the master has seen its presence.
			m_espman->m_masterAcknowledged.store(true, std::memory_order_release);

			// Do not re-send the message if it was intended for me(m_espman->m_myMAC == MessagesMap::parseMacAddress()).
			return;
		}
		else if (msg.m_msgType == MessageType::MSG_MASTER_REQ_ACK && m_espman->m_myMAC == MessagesMap::parseMacAddress(msg.m_data.macAddress)) {
			// The master ESP does not know about the current ESP. The current ESP must announce itself to the master.
			m_espman->m_masterAcknowledged.store(false, std::memory_order_release);
			return;
		}

	}
	
	if (msg.m_msgType == MessageType::MSG_STOP_ALARM){
			//TODO
	}

	// Broadcast every received peer message(because I am a slave). 
	MessageRaw transmitMsg{};
	const int length = prepareMessageForTransmission(msg, transmitMsg);
	m_espman->sendData(reinterpret_cast<const uint8_t*>(&transmitMsg), length);
}

SlaveCallbackSelf::SlaveCallbackSelf(EspNowManager &espman)
	: m_espman(&espman) {

}

void SlaveCallbackSelf::operator()(const Message &msg) {
	if (!m_espman) {
		return;
	}

	// Just send my message to my peers.
	MessageRaw transmitMsg{};
	const int length = prepareMessageForTransmission(msg, transmitMsg);
	m_espman->sendData(reinterpret_cast<const uint8_t*>(&transmitMsg), length);
}
