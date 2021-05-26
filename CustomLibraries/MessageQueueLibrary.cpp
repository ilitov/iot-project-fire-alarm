#include "MessageQueueLibrary.h"

MessageQueue::MessageQueue()
	: m_readIdx(0)
	, m_writeIdx(0) {

}

bool MessageQueue::pop(Message &result) {
	const int readIdx = m_readIdx.load(std::memory_order_relaxed);
	const int writeIdx = m_writeIdx.load(std::memory_order_acquire);

	if (readIdx == writeIdx) {
		return false;
	}

	result = m_messages[readIdx];

	const int nextReadIdx = nextIndex(readIdx);
	m_readIdx.store(nextReadIdx, std::memory_order_release);

	return true;
}

bool MessageQueue::push(const Message &msg) {
	const int writeIdx = m_writeIdx.load(std::memory_order_relaxed);
	const int nextWriteIdx = nextIndex(writeIdx);
	
	const int readIdx = m_readIdx.load(std::memory_order_acquire);

	if (nextWriteIdx == readIdx) {
		return false;
	}

	m_messages[writeIdx] = msg;

	m_writeIdx.store(nextWriteIdx, std::memory_order_release);

	return true;
}

int MessageQueue::nextIndex(int value) {
	return (value + 1) % SIZE;
}


// ########### TESTS ###########

/*

#include <iostream>
#include <thread>
#include <chrono>

void f(MessageQueue &q) {
	int i = 0;

	Message msg;

	while (i <= 1000) {
		msg.m_msgId = i;
		while (!q.push(msg));
		//std::cout << "Write " << msg.m_msgId << '\n';
		++i;
	}
}

void g(MessageQueue &q) {
	Message msg;
	msg.m_msgId = 0;

	int last = -1;

	auto s = std::chrono::steady_clock::now();

	while (msg.m_msgId < 1000) {
		while (!q.pop(msg));

		//std::cout << "Read " << msg.m_msgId << '\n';

		if (last + 1 != msg.m_msgId) {
			abort();
		}

		last = msg.m_msgId;
	}

	auto e = std::chrono::steady_clock::now();
	std::cout << "Total read time: " << std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count() << " ms.\n";
}

int main() {
	MessageQueue q;
	std::thread t1(g, std::ref(q));
	std::thread t2(f, std::ref(q));

	t1.join();
	t2.join();

	return 0;
}

*/
