#include "MessagesProcessor.h"


MessagesProcessorBase::MessagesProcessorBase()
	: m_hasWork(false)
	, m_terminated(false) {

}

MessagesProcessorBase::~MessagesProcessorBase() {
	terminate();
	m_hasWork = false;
	m_terminated = false;
}

bool MessagesProcessorBase::push(const Message &msg) {
	const bool result = m_queue.push(msg);

	if (result) {
		std::lock_guard<std::mutex> lock(m_mtx);
		m_hasWork = true;
		m_doWork.notify_one();
	}

	return result;
}

void MessagesProcessorBase::terminate() {
	{
		std::lock_guard<std::mutex> lock(m_mtx);
		m_terminated = true;
	}
	
	m_doWork.notify_one();
}

template <typename Callback>
MessagesProcessor<Callback>::MessagesProcessor(Callback &&callback)
	: m_callback(std::move(callback)) {

}

template <typename Callback>
void MessagesProcessor<Callback>::process() {
	Message msg;

	std::unique_lock<std::mutex> lock(m_mtx);

	while (!m_terminated) {
		// Wait if there are no messages to process.
		m_doWork.wait(lock, [this]() { return m_terminated || m_hasWork; });

		// Stop if signaled.
		if (m_terminated) {
			return;
		}

		do {
			m_hasWork = m_queue.pop(msg);

			if (m_hasWork) {
				m_callback(msg);
			}
		} while (m_hasWork && !m_terminated);
	}
}

template class MessagesProcessor<MasterCallbackPeers>;
template class MessagesProcessor<SlaveCallbackPeers>;
template class MessagesProcessor<SlaveCallbackSelf>;
