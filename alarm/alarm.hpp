#ifndef ALARM_HEADER_INCLUDED
#define ALARM_HEADER_INCLUDED

#include <thread>
#include <condition_variable>
#include <atomic>

class Alarm
{
  public:
    Alarm(const int pin)
    {
      ledcSetup(CHANNEL, INITIAL_FREQUENCY, INITIAL_RESOLUTION);
      ledcAttachPin(pin, CHANNEL);
    }

    ~Alarm()
    {
      if (m_isActive == true)
      {
        m_isActive = false;
        m_thread.join();
      }
    }

    void activate()
    {
      m_isActive = true;
      m_thread = std::thread(&Alarm::threadFunction, this);
    }

    void deactivate()
    {
      m_isActive = false;
      m_thread.join();
    }

  private:

    void threadFunction()
    {
      while(m_isActive)
      {
        ledcWriteTone(CHANNEL, TRIGGER_FREQUENCY);
        delay(DELAY_TIME_MS);
        ledcWriteTone(CHANNEL, INITIAL_FREQUENCY);
        delay(DELAY_TIME_MS);
      }
    }
  
    const uint8_t CHANNEL = 0;
    const uint8_t INITIAL_RESOLUTION = 0;
    const double INITIAL_FREQUENCY = 0;
    const double TRIGGER_FREQUENCY = 4500;
    const int DELAY_TIME_MS = 1000;

    std::thread m_thread;
    std::atomic_bool m_isActive {false};
};

#endif // ALARM_HEADER_INCLUDED
