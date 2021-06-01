#ifndef _HARD_RESET_HEADER_
#define _HARD_RESET_HEADER_

#include "Timer.h"
#include <mutex>

// Deletes the ESP settings file if the BOOT button is held down for N seconds.
class HardReset {
public:
	static const unsigned long SECONDS = 3 * 1000;

	static HardReset& instance(int FLASH_BUTTON = 0);

	void setup();
	
	// Call it in the loop() function to reset the state if needed.
	void kick();

	static void handleInterrupt();

private:
	HardReset(int FLASH_BUTTON);

	volatile bool m_reset;
	const int m_button;
	Timer m_time;
	std::mutex m_mtx;
};

#endif // !_HARD_RESET_HEADER_
