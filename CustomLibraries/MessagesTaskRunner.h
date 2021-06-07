#ifndef _MESSAGES_TASK_RUNNER_
#define _MESSAGES_TASK_RUNNER_

#include <array>
#include <thread>

#include "MessagesProcessor.h"

class MessagesTaskRunner {
public:
	// Two tasks: 1 for the local messages and 1 for the messages that we receive.
	static const int MAX_TASK_COUNT = 2;
	MessagesTaskRunner();
	MessagesTaskRunner(const MessagesTaskRunner&) = delete;
	MessagesTaskRunner& operator=(const MessagesTaskRunner&) = delete;
	~MessagesTaskRunner();

	bool push(MessagesProcessorBase *task);
	void joinTasks();

private:
	int m_index;
	std::array<std::thread, MAX_TASK_COUNT> m_tasks;
	std::array<MessagesProcessorBase*, MAX_TASK_COUNT> m_tasksPtr;
};

#endif // !_MESSAGES_TASK_RUNNER_
