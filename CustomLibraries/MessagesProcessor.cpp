#include "MessagesProcessor.h"


MessagesProcessorBase::MessagesProcessorBase()
	: m_terminated(false) {

}

MessagesProcessorBase::~MessagesProcessorBase() {
	terminate();
}

bool MessagesProcessorBase::push(const Message &msg) {
	const bool result = m_queue.push(msg);

	if (result) {
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
	bool hasWork = true;
	std::unique_lock<std::mutex> lock(m_mtx);

	while (!m_terminated) {
		// Wait if there are no messages to process.
		if (!hasWork) {
			m_doWork.wait(lock);
		}

		// Stop if signaled.
		if (m_terminated) {
			return;
		}

		do {
			hasWork = m_queue.pop(msg);

			if (hasWork) {
				m_callback(msg);
			}
		} while (hasWork);
	}
}

template class MessagesProcessor<MasterCallbackPeers>;
template class MessagesProcessor<SlaveCallbackPeers>;
template class MessagesProcessor<SlaveCallbackSelf>;
