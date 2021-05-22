#include "MessagesProcessorMaster.h"

MessagesProcessor::MessagesProcessor(MessagesProcessor::MessageCallback callback)
	: m_callback(callback)
	, m_terminated(false) {

}

MessagesProcessor::~MessagesProcessor() {
	terminate();
}

bool MessagesProcessor::push(const Message &msg) {
	const bool result = m_queue.push(msg);

	if (result) {
		m_doWork.notify_one();
	}

	return result;
}

void MessagesProcessor::process() {
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

void MessagesProcessor::terminate() {
	std::lock_guard<std::mutex> lock(m_mtx);
	m_terminated = true;
}
