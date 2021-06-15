#include "TelegramBot.h"
#include "EspNowManager.h"

// Initialize Telegram BOT(Get from Botfather)
const char *TelegramBot::BOTToken = "1887265752:AAEB7UvNeanC5WMUb2kle6U6eUG5-Rb-ybo";

TelegramBot::TelegramBot()
	: m_client()
	, m_bot(BOTToken, m_client)
	, m_task(NULL)
	, m_run(false) {
	m_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
}

TelegramBot::~TelegramBot() {
	stop();
}

TelegramBot& TelegramBot::instance() {
	static TelegramBot instance;
	return instance;
}

bool TelegramBot::postMACAddress(MessagesMap::mac_t mac) {
	return m_queue.push(QueueType{ mac, millis() });
}

void TelegramBot::handleTelegramBot() {
	const int numNewMessages = m_bot.getUpdates(m_bot.last_message_received + 1);

	if (numNewMessages > 0) {
		Serial.println("Got response.");
		handleNewMessages(numNewMessages);
	}
}

void TelegramBot::handleNewMessages(int numNewMessages) {
	Serial.print("numNewMessages: ");
	Serial.println(numNewMessages);

	for (int i = 0; i < numNewMessages; ++i) {
		// Chat id of the requester
		const String &chat_id = String(m_bot.messages[i].chat_id);

		if (0 != strcmp(chat_id.c_str(), ESPSettings::instance().getTelegramChatID())) {
			m_bot.sendSimpleMessage(chat_id, "Unauthorized user", "");
			continue;
		}

		// Print the received message
		const String &text = m_bot.messages[i].text;
		Serial.println(text);

		// Command template: /pause <staq> <time>
		if (text.startsWith("/pause")) {
			static int msgId = 0;
			EspNowManager &espMan = EspNowManager::instance();

			const std::string &room = parseCmdValue(text, ' ', 1).c_str();
			unsigned long alarmPauseDuration = static_cast<unsigned long>(parseCmdValue(text, ' ', 2).toInt());

			auto it = espMan.m_slavesMapToMAC.find(room);

			// If there is no such room, continue.
			if (it == espMan.m_slavesMapToMAC.end()) {
				continue;
			}

			Message m;
			MessagesMap::parseMacAddressReadable(WiFi.macAddress().c_str(), m.m_mac);
			m.m_msgType = MessageType::MSG_STOP_ALARM;
			m.m_alarmStatus = AlarmType::ALARM_INVALID;
			m.m_msgId = msgId++;

			m.m_data.stopInformation.stopDurationMS = alarmPauseDuration * 1000; // alarmPauseDuration sec * 1000 ms
			MessagesMap::parseMacAddress(it->second, m.m_data.stopInformation.macAddress);

			if (espMan.enqueueSendDataAsync(m)) {
				auto timeoutIt = m_peersNotifications.find(it->second);

				// Reset the timer and change the timeout duration.
				if (timeoutIt != m_peersNotifications.end()) {
					timeoutIt->second.m_timer.reset();
					timeoutIt->second.m_currentTimeout = m.m_data.stopInformation.stopDurationMS;
				}

				const String toBotMessage = "Successfully stopped the fire alarm for " + String(alarmPauseDuration) + " seconds!";
				sendMessage(toBotMessage);
			}
		}
	}
}

bool TelegramBot::sendMessage(const String &message) {
	/*Serial.println("Sending message:");
	Serial.println(ESPSettings::instance().getTelegramChatID());
	Serial.println(message);*/

	return m_bot.sendSimpleMessage(ESPSettings::instance().getTelegramChatID(), message, "");
}

String TelegramBot::parseCmdValue(const String &data, char separator, int index) {
	int found = 0;
	int strIndex[2] = { 0, -1 };
	const int maxIndex = (int)data.length() - 1;

	for (int i = 0; i <= maxIndex && found <= index; i++) {
		if (data.charAt(i) == separator || i == maxIndex) {
			found++;
			strIndex[0] = strIndex[1] + 1;
			strIndex[1] = (i == maxIndex) ? i + 1 : i;
		}
	}

	return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void TelegramBot::taskFunction(void *taskParams) {
	EspNowManager &espMan = EspNowManager::instance();
	TelegramBot &bot = instance();

	const unsigned long DEFAULT_NOTIFICATION_TIMEOUT = 30 * 1000; // 30 sec

	while (bot.m_run.load()) {

		// Handle the messages from the bot to the ESP.
		Serial.println("Handle Telegram bot.");
		bot.handleTelegramBot();

		bool stopMessagesProcessing = false;

		while (!stopMessagesProcessing) {
			// Handle the messages from the ESP to the bot.
			QueueType msgInfo;

			if (bot.m_queue.pop(msgInfo)) {
				auto it = espMan.m_slavesMapToName.find(msgInfo.m_mac);

				if (it == espMan.m_slavesMapToName.end()) {
					continue;
				}

				auto timeoutIt = bot.m_peersNotifications.find(msgInfo.m_mac);

				// If there is a record.
				if (timeoutIt != bot.m_peersNotifications.end()) {

					// Drop the message.
					if (timeoutIt->second.m_timer.elapsedTime() < timeoutIt->second.m_currentTimeout) {
						//Serial.println("The elapsed time since the last notification is less than the timeout.");
						//Serial.println("The alarm notification for room " + it->second + " has NOT been sent to Telegram bot.");
						continue;
					}
					// Drop the message again.
					else if (msgInfo.m_timeInserted <= timeoutIt->second.m_timer.time()) {
						//Serial.println("Outdated message, the alarm has already been deactivated.");
						//Serial.println("The alarm notification has NOT been sent to Telegram bot.");
						continue;
					}

				}

				const String message = "The fire alarm of ESP " + it->second + " has been activated.";

				if (bot.sendMessage(message)) {

					// Insert a new record in the map.
					if (timeoutIt == bot.m_peersNotifications.end()) {
						bot.m_peersNotifications[msgInfo.m_mac] = NotificationTimeout{ Timer(), DEFAULT_NOTIFICATION_TIMEOUT };
					}
					else {
						// Reset the timeout timer if the message has been sent successfully.
						timeoutIt->second.m_timer.reset();
						timeoutIt->second.m_currentTimeout = DEFAULT_NOTIFICATION_TIMEOUT;
					}

					Serial.println("Alarm notification has been sent to Telegram bot.");
				}
			}

			stopMessagesProcessing = true;
		}

		delay(1500);
	}
}

void TelegramBot::begin() {
	m_run.store(true);

	BaseType_t res = xTaskCreate(
	&TelegramBot::taskFunction,
	"bot_task" /* name */,
	2048 + 4096 /* stack size */,
	NULL /* parameters */,
	1 /* priority */,
	&m_task);

	if (res != pdPASS) {
		Serial.println("Could not create the task which handles Telegram bot. Restarting...");
		esp_restart();
	}
}

void TelegramBot::stop() {
	m_run.store(false);

	if (m_task) {
		vTaskDelete(m_task);
		m_task = NULL;
	}
}
