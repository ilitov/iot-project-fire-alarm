#ifndef _MESSAGES_PROCESSOR_MASTER
#define _MESSAGES_PROCESSOR_MASTER

#include <mutex>
#include <condition_variable>

#include "../MessageQueueLibrary/MessageQueueLibrary.h"

// A class that manages the received messages in the master ESP.
class MessagesProcessor {
public:
	// Make it a template parameter(bigger executable, avoid templates)? Change its signature?
	typedef void(*MessageCallback)(const Message &msg);

	MessagesProcessor(MessageCallback callback);
	MessagesProcessor(const MessagesProcessor &) = delete;
	MessagesProcessor& operator=(const MessagesProcessor &) = delete;
	~MessagesProcessor();

	bool push(const Message &msg);

	// This function runs on a separate thread?
	void process();

	void terminate();

private:
	MessageCallback m_callback;
	MessageQueue m_queue;

	bool m_terminated;
	std::mutex m_mtx;
	std::condition_variable m_doWork;
};

#endif // !_MESSAGES_PROCESSOR_MASTER