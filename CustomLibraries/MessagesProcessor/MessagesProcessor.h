#ifndef _MESSAGES_PROCESSOR_MASTER
#define _MESSAGES_PROCESSOR_MASTER

#include <mutex>
#include <condition_variable>

#include "../MessageQueueLibrary/MessageQueueLibrary.h"
#include "../MessagesProcessorCallbacks/MessagesCallbacks.h"

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

protected:
	MessageQueue m_queue;

	bool m_terminated;
	std::mutex m_mtx;
	std::condition_variable m_doWork;
};

// Use two different callbacks depending on the type of the ESP(master/slave).
template <typename Callback>
class MessagesProcessor : MessagesProcessorBase {
public:
	MessagesProcessor(Callback &&callback);
	MessagesProcessor(const MessagesProcessor &) = delete;
	MessagesProcessor& operator=(const MessagesProcessor &) = delete;
	~MessagesProcessor() = default;

	// This function runs on a separate thread?
	void process();

private:
	Callback m_callback;
};

typedef MessagesProcessor<MasterCallback> MasterProcessor;
typedef MessagesProcessor<SlaveCallback> SlaveProcessor;

#endif // !_MESSAGES_PROCESSOR_MASTER
