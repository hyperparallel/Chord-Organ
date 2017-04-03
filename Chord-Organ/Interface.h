#ifndef Interface_h
#define Interface_h

class Interface;

class LedBar {
public:

    LedBar(int pin0, int pin1, int pin2, int pin3);
    void ledWrite(int num);
    void ledWriteBit(int num, bool overRide = false);
    void update();
private:
    int _pin0;
    int _pin1;
    int _pin2;
    int _pin3;
    int _ledStore = 0;
    int _overRideStore = 0;
    bool _overRide = false;
};

class Button {
public:

    Button (int pin, int shortPressDuration, int longPressDuration);

    void setButtonState(int buttonState, Interface& interface);
    void update();
    bool shortPress;
    bool longPress;
    //bool state;

private:
    //const unsigned int MAXIMUM = 256;
    void _debounce(unsigned int input);
    int _pin;
    uint _shortPressDuration;
    uint _longPressDuration;
    int _buttonState = 0;
    elapsedMillis _buttonTimer;
    unsigned int _integrator;
};


// get the button going next

class Input {
public:

    explicit Input(int pin);

    void setup(int numOfSteps, int percentHyst, int ADC_MAX_VAL, int isCV = false);
    void update();
    //void updateCV();
    void setVOctCal(int vOctCal);
    void setIsCV(int isCV);
    bool isMenuB = false;
    bool changed = false;

    int quantVal = 0; // quantised value
    int value = 0; // software position
    int smoothVal = 0; // averaged raw position
    int rawVal; // unprocessed raw reading

    // Debug functions
    void printSetup();
    void printStatus();
    //virtual void setLockOut(bool);

protected:
    int _pin;
    //int _updatePosition(int valueNew, int valueOld, int stepSize, int hysteresis);
    int _updatePosition();
    int _rollingAvg(int rawValue);
    bool _isHighRes = false;
    bool _isCV = false;

    int _value = 0;
    int _quantVal = 0;
    int _vOctCal = 0;  // adjust step size by a small amount

    int _stepSize;
    int _quarterStep;
    float _coeff;
    int _hysteresis;

    int _avgSum = 0;
    int _avgCounter = 0;
    int _avgPot[128];

    bool _lockedOut;
    int _lockValue = 0;
    bool _rawValStored = false;
    int _rawValDiff = 0;
    elapsedMillis _overRideTimer;
    bool _overRideStatus = false;
};

class InputPot: public Input {
public:
    explicit InputPot(int pin) : Input(pin) {}
    void update();
};
/*
class Interface {
public:

    //Interface();
    void update();
        
protected:
    virtual void setLockOut(bool lockOut);
    virtual bool getLockOut();

    //friend void Button::setButtonState(int buttonState, Interface& interface);
    int getButtonState();
    
    bool _lockOut = false;
    int _buttonState;
};
*/

/*
class stdPot: public virtual Pot {
 public:

  explicit stdPot(int pin): Pot(pin) {}

  bool update();
  bool calcIndex();
  int rawValue = 0;

 private:
  int _index; //sofware position
  int _rawValue; // actual position currently
  int _rawValueOld;
  int _bounceValue; //rawValue debounced
};

class vOctPot: public virtual Pot {
 public:

  explicit vOctPot(int pin): Pot(pin) {}

  bool update();
  bool calcIndex();
  int rawValue = 0;

 private:
  int _index; //sofware position
  int _rawValue; // actual position currently
  int _rawValueOld;
  int _bounceValue; //rawValue debounced
};


class CVJack {
 public:

  CVJack(int pin);
  bool update();
  int mode;

 private:
  int _pin;
  int _rawValue;
  int _mode;
};


class LedBar {
 public:

  LedBar(int pin0, int pin1, int pin2, int pin3);
  bool update();
  void startup();
  void larsonScanStart(int timer);
  void larsonScanStop();
  void larsonScan();
  void ledOn(int pin); //should make these private
  void ledOff(int pin);
  void ledAction(int pin, bool status);
  void writeLeds(short pattern);
  void writeStatus16(short);
  void setWaveform(short);
  void blinkWaveform();
  void dimPosition();
    
 private:
  void larsonRun();
  int _pin[4];
  bool scanner = 0;
  elapsedMillis scannerTimer;
  elapsedMillis scannerClock;
  int scannerCount;
  unsigned int scanTime;
  short _waveform;
  short _xPattern;
  short _pattern;
  elapsedMillis _blinkTimer;
  int _blinkLength;
  bool _blinkStatus;
  bool _dimStatus;
  int _dutyClock = 0;
  int _dutyCycle;
  
  

  short larsonPat[14] = {
    0b0000,
    0b0000,
    0b1000,
    0b1100,
    0b0110,
    0b0011,
    0b0001,
    0b0000,
    0b0000,
    0b0001,
    0b0011,
    0b0110,
    0b1100,
    0b1000};

  short status16[16] = {
    0b0000,
    0b1000,
    0b1100,
    0b0100,
    0b1010,
    0b1110,
    0b1101,
    0b0110,
    0b1111,
    0b1001,
    0b1011,
    0b0111,
    0b0101,
    0b0010,
    0b0011,
    0b0001};

};
*/



#endif
