#ifndef _MESSAGES_CALLBACKS_
#define _MESSAGES_CALLBACKS_

#include <HardwareSerial.h>

#include "../MessageManagerLibrary/MessageLibrary.h"

class EspNowManager;

class MasterCallback {
public:
	void operator()(const Message &msg);
};

class SlaveCallback {
public:
	SlaveCallback(EspNowManager &espman);

	void operator()(const Message &msg);

private:
	EspNowManager *m_espman;
};

#endif // !_MESSAGES_CALLBACKS_
