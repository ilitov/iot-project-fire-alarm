#ifndef ALARM_HEADER_INCLUDED
#define ALARM_HEADER_INCLUDED

#include <thread>
#include <atomic>
#include <condition_variable>

class Alarm
{
  public:
    Alarm(const int pin)
    {
      ledcSetup(CHANNEL, INITIAL_FREQUENCY, INITIAL_RESOLUTION);
      ledcAttachPin(pin, CHANNEL);

      m_thread = std::thread(&Alarm::threadFunction, this);
    }

    ~Alarm()
    {
      m_state = State::KILLED;
      m_condVar.notify_one();
      m_thread.join();
    }

    void activate()
    {
      m_state = State::ACTIVATED;
      m_condVar.notify_one();
    }

    void deactivate()
    {
      m_state = State::DEACTIVATED;
    }

  private:

    enum class State
    {
      ACTIVATED = 0,
      DEACTIVATED,
      KILLED
    };

    void threadFunction()
    {
      while(m_state != State::KILLED)
      {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condVar.wait(lock, [this] { return m_state != State::DEACTIVATED; });

        while(m_state == State::ACTIVATED)
        {
          ledcWriteTone(CHANNEL, TRIGGER_FREQUENCY);
          delay(DELAY_TIME_MS);
          ledcWriteTone(CHANNEL, INITIAL_FREQUENCY);
          delay(DELAY_TIME_MS);
        }
      }
    }
  
    const uint8_t CHANNEL = 0;
    const uint8_t INITIAL_RESOLUTION = 0;
    const double INITIAL_FREQUENCY = 0;
    const double TRIGGER_FREQUENCY = 4500;
    const int DELAY_TIME_MS = 1000;

    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_condVar;
    std::atomic<State> m_state {State::DEACTIVATED};
};

#endif // ALARM_HEADER_INCLUDED
