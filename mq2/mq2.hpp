#ifndef MQ2_HEADER_INCLUDED
#define MQ2_HEADER_INCLUDED

class MQ2
{
  public:
    MQ2(int pin)
      : m_pin(pin)
    {
      pinMode(pin, INPUT);
    }

    // Returns a value between 0 and 4095
    int readGasConcentration()
    {
      return analogRead(m_pin);
    }

  private:
    int m_pin;
};

#endif // MQ2_HEADER_INCLUDED
