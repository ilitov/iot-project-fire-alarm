#ifndef _MESSAGES_PROCESSOR_MASTER
#define _MESSAGES_PROCESSOR_MASTER

#include <mutex>
#include <condition_variable>

#include "../MessageQueueLibrary/MessageQueueLibrary.h"

// A class that manages the received messages in the master ESP.
class MessagesProcessorBase {
public:
	MessagesProcessorBase();
	MessagesProcessorBase(const MessagesProcessorBase &) = delete;
	MessagesProcessorBase& operator=(const MessagesProcessorBase &) = delete;
	virtual ~MessagesProcessorBase();

	bool push(const Message &msg);
	virtual void process() = 0;
	void terminate();

private:
	MessageQueue m_queue;

	bool m_terminated;
	std::mutex m_mtx;
	std::condition_variable m_doWork;
};

// Use two different callbacks depending on the type of the ESP(master/slave).
template <typename Callback>
class MessagesProcessor {
public:
	// Make it a template parameter(bigger executable, avoid templates)? Change its signature?
	typedef void(*MessageCallback)(const Message &msg);

	MessagesProcessor(Callback &&callback)
		: m_callback(std::move(callback)) {

	}
	MessagesProcessor(const MessagesProcessor &) = delete;
	MessagesProcessor& operator=(const MessagesProcessor &) = delete;
	~MessagesProcessor() = default;

	// This function runs on a separate thread?
	void process() {
		Message msg;
		std::unique_lock<std::mutex> lock(m_mtx);

		while (!m_terminated) { // lol
			m_doWork.wait(lock);

			// Stop if signaled.
			if (m_terminated) {
				return;
			}

			bool runs = false;
			do {
				runs = m_queue.pop(msg);

				if (runs) {
					m_callback(msg);
				}
			} while (runs);
		}
	}

private:
	Callback m_callback;
};

#endif // !_MESSAGES_PROCESSOR_MASTER
