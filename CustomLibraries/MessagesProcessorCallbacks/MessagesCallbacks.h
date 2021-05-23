#ifndef _MESSAGES_CALLBACKS_
#define _MESSAGES_CALLBACKS_

#include <HardwareSerial.h>

#include "../MessageManagerLibrary/MessageLibrary.h"
#include "../EspNowManagerLibrary/EspNowManager.h"

class EspNowManager;

class MasterCallback {
public:
	MasterCallback() = default;
	MasterCallback(const MasterCallback&) = delete;
	MasterCallback& operator=(const MasterCallback&) = delete;
	~MasterCallback() = default;

	void operator()(const Message &msg);
};

class SlaveCallback {
public:
	SlaveCallback(EspNowManager &espman);
	SlaveCallback(const SlaveCallback&) = delete;
	SlaveCallback& operator=(const SlaveCallback&) = delete;
	~SlaveCallback() = default;

	void operator()(const Message &msg);

private:
	EspNowManager *m_espman;
};

#endif // !_MESSAGES_CALLBACKS_
