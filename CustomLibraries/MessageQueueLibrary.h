#ifndef _MESSAGE_QUEUE_LIBRARY_
#define _MESSAGE_QUEUE_LIBRARY_

#include <atomic>

#include "../MessageManagerLibrary/MessageLibrary.h"

// Lock-free queue that supports 1 reader and 1 writer.
class MessageQueue {
public:	
	static const unsigned int SIZE = 128;

	explicit MessageQueue();
	MessageQueue(const MessageQueue &) = delete;
	MessageQueue& operator=(const MessageQueue &) = delete;
	~MessageQueue() = default;

public:
	bool pop(Message &result);
	bool push(const Message &msg);

private:
	static int nextIndex(int value);

private:
	Message m_messages[SIZE];
	std::atomic<int> m_readIdx;
	std::atomic<int> m_writeIdx;
};

#endif // !_MESSAGE_QUEUE_LIBRARY_
