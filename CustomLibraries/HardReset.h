#ifndef _HARD_RESET_HEADER_
#define _HARD_RESET_HEADER_

#include "Timer.h"
#include <atomic>
#include <thread>

// Deletes the ESP settings file if the BOOT button is held down for N seconds.
class HardReset {
public:
	static const unsigned long HOLD_SECONDS = 3 * 1000;

	static HardReset& instance(int FLASH_BUTTON = 0);

	void setup();
	void disable();
	
	// Call it in the loop() function to reset the state if needed.
	void kick();

	static void handleInterrupt();

private:
	HardReset(int FLASH_BUTTON);
	~HardReset();

	volatile std::atomic<bool> m_reset;
	const int m_button;
	Timer m_time;
	std::thread m_thread;
	std::atomic_flag m_run;
};

#endif // !_HARD_RESET_HEADER_
