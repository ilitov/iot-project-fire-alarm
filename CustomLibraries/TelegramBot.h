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

class TelegramBot {
private:
	static const char *BOTToken;

	TelegramBot();
	void handleNewMessages(int numNewMessages);
	void handleTelegramBot();
	void sendMessage(const String &message);
	String parseCmdValue(const String &data, char separator, int index);

	static void taskFunction(void *taskParams = nullptr);

public:
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
	Queue<MessagesMap::mac_t, 128> m_queue;
};

#endif // !_TELEGRAM_BOT_HEADER_
