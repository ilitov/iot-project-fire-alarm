#ifndef _MESSAGES_CALLBACKS_
#define _MESSAGES_CALLBACKS_

#include <HardwareSerial.h>

#include "../MessageManagerLibrary/MessageLibrary.h"

class EspNowManager;

class MasterCallbackPeers {
public:
	MasterCallbackPeers(EspNowManager &espman);

	void operator()(const Message &msg);

private:
	EspNowManager *m_espman;
};

class SlaveCallbackPeers {
public:
	SlaveCallbackPeers(EspNowManager &espman);

	void operator()(const Message &msg);

private:
	EspNowManager *m_espman;
};

class SlaveCallbackSelf {
public:
	SlaveCallbackSelf(EspNowManager &espman);

	void operator()(const Message &msg);

private:
	EspNowManager *m_espman;
};

#endif // !_MESSAGES_CALLBACKS_
