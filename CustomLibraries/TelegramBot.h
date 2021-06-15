#ifndef _TELEGRAM_BOT_HEADER_
#define _TELEGRAM_BOT_HEADER_

#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <WiFiClientSecure.h>
#include "QueueLibrary.h"
#include "PeersMessagesLibrary.h"
#include "Timer.h"
#include "EspSettings.h"
#include <thread>
#include <atomic>
#include <map>

class TelegramBot {
	struct NotificationTimeout {
		Timer m_timer;
		unsigned long m_currentTimeout;
	};

private:
	static const char *BOTToken;

	TelegramBot();
	void handleNewMessages(int numNewMessages);
	void handleTelegramBot();
	bool sendMessage(const String &message);
	String parseCmdValue(const String &data, char separator, int index);

	static void taskFunction(void *taskParams = nullptr);

public:
	struct QueueType {
		MessagesMap::mac_t m_mac;
		unsigned long m_timeInserted;
	};

	~TelegramBot();

	static TelegramBot& instance();

	bool postMACAddress(MessagesMap::mac_t mac);
	   
	void begin();
	void stop();

private:
	WiFiClientSecure m_client;
	UniversalTelegramBot m_bot;
	TaskHandle_t m_task;
	std::atomic<bool> m_run;
	Queue<QueueType, 128> m_queue;
	std::map<MessagesMap::mac_t, NotificationTimeout> m_peersNotifications;
};

#endif // !_TELEGRAM_BOT_HEADER_
