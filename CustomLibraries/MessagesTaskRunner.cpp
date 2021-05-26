#include "MessagesTaskRunner.h"

MessagesTaskRunner::MessagesTaskRunner()
	: m_index(0) {

}

MessagesTaskRunner::~MessagesTaskRunner() {
	joinTasks();
}

bool MessagesTaskRunner::push(MessagesProcessorBase *task) {
	if (m_index >= MAX_TASK_COUNT) {
		return false;
	}

	m_tasks[m_index] = std::thread(&MessagesProcessorBase::process, task);
	++m_index;

	return true;
}

void MessagesTaskRunner::joinTasks() {
	for (int i = 0; i < m_index; ++i) {
		if (m_tasks[i].joinable()) {
			m_tasks[i].join();
		}
	}
}
