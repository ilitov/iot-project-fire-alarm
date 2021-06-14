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
	return m_queue.push(mac);
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

		static int msgId = 0;

		// Command template: /pause <staq> <time>
		if (text.startsWith("/pause")) {
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
				const String toBotMessage = "Successfully stopped the fire alarm for " + String(alarmPauseDuration) + " seconds!";
				sendMessage(toBotMessage);
			}
		}
	}
}

void TelegramBot::sendMessage(const String &message) {
	m_bot.sendSimpleMessage(ESPSettings::instance().getTelegramChatID(), message, "");
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

	while (bot.m_run.load()) {

		// Handle the messages from the bot to the ESP.
		Serial.println("Handle Telegram bot.");
		bot.handleTelegramBot();

		// Handle the messages from the ESP to the bot.
		MessagesMap::mac_t authorMAC;

		if (bot.m_queue.pop(authorMAC)) {
			auto it = espMan.m_slavesMapToName.find(authorMAC);

			if (it != espMan.m_slavesMapToName.end()) {
				const String &message = "The fire alarm of ESP " + it->second + " has been activated.";

				bot.sendMessage(message);

				Serial.println("Alarm notification has been sent to Telegram bot.");

				continue;
			}
		}

		delay(2000);
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
