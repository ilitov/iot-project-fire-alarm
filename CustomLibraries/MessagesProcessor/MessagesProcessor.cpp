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
	std::lock_guard<std::mutex> lock(m_mtx);
	m_terminated = true;
}
