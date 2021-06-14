#ifndef _QUEUE_LIBRARY_
#define _QUEUE_LIBRARY_

#include <atomic>

#include "MessageLibrary.h"

// Lock-free queue that supports 1 reader and 1 writer.
template <typename T, unsigned int SIZE>
class Queue {
public:
	explicit Queue()
		: m_readIdx(0)
		, m_writeIdx(0) {

	}

	Queue(const Queue &) = delete;
	Queue& operator=(const Queue &) = delete;
	~Queue() = default;

public:
	bool pop(T &result) {
		const int readIdx = m_readIdx.load(std::memory_order_relaxed);
		const int writeIdx = m_writeIdx.load(std::memory_order_acquire);

		if (readIdx == writeIdx) {
			return false;
		}

		result = m_data[readIdx];

		const int nextReadIdx = nextIndex(readIdx);
		m_readIdx.store(nextReadIdx, std::memory_order_release);

		return true;
	}

	bool push(const T &value) {
		const int writeIdx = m_writeIdx.load(std::memory_order_relaxed);
		const int nextWriteIdx = nextIndex(writeIdx);

		const int readIdx = m_readIdx.load(std::memory_order_acquire);

		if (nextWriteIdx == readIdx) {
			return false;
		}

		m_data[writeIdx] = value;

		m_writeIdx.store(nextWriteIdx, std::memory_order_release);

		return true;
	}

private:
	static int nextIndex(int value) {
		return (value + 1) % SIZE;
	}

private:
	T m_data[SIZE];
	std::atomic<int> m_readIdx;
	std::atomic<int> m_writeIdx;
};

#endif // !_QUEUE_LIBRARY_
