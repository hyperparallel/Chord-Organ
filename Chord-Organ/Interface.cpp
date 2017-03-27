#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#endif
#include "Interface.h"

#define INTEGRATOR_MAXIMUM 256

bool interfaceButtonState = 0;
bool interfaceLockOut = 0;
bool ledOverRide = 0;
short ledString = 0;

LedBar::LedBar(int pin3, int pin2, int pin1, int pin0) {
  _pin3 = pin3;
  _pin2 = pin2;
  _pin1 = pin1;
  _pin0 = pin0;
}

void LedBar::ledWriteBit(int n, bool overRide /* = false */) {

    if (_overRide && !overRide) {
        // do not update
        // keep led value updated
        _overRideStore = n;
    }else if (n != _ledStore) {
        digitalWrite(_pin3, n & 0b1000);
        digitalWrite(_pin2, n & 0b0100);
        digitalWrite(_pin1, n & 0b0010);
        digitalWrite(_pin0, n & 0b0001);
        _ledStore = n;
    }
}

void LedBar::update() {
    if (ledOverRide) {
        _overRide = true;
        if (_overRideStore == 0) _overRideStore = _ledStore;
        ledWriteBit(ledString, 1);
    }else {
        _overRide = false;
        if (_overRideStore != 0) {
            ledWriteBit(_overRideStore);
            _overRideStore = 0;
        }
    }
}

//--------------------
// Define an Interface
//--------------------
/*
//Interface::Interface() {}
void Interface::update() {
    Serial.print("buttonState ");
    Serial.println(_buttonState);
}

bool Interface::getLockOut() {
    return _lockOut;
}

void Interface::setLockOut(bool lockOut) {
    _lockOut = lockOut;
}

int Interface::getButtonState() {
    //Serial.print("buttonState ");
    Serial.println(_buttonState);
    return _buttonState;
}
*/

//--------------------
// Define an Input
//--------------------

Input::Input (int pin) {
    _pin = pin;
}

void InputPot::update() {

    bool buttonPressed = interfaceButtonState;

    changed = false;

    // read in value
    rawVal = analogRead(_pin);
    smoothVal = _rollingAvg(rawVal);

    // Increase resolution for 1v/oct and small step sizes
    if (_isHighRes) {
        smoothVal <<= 2; // multiply by 4
    }

    value = _updatePosition();

    // Turn off override leds after 2.5 seconds of no activity
    if (_overRideTimer > 1000 && _overRideStatus && !isMenuB) {
        ledOverRide = false;
        _overRideStatus = false;
    }
                
    
        // is the button down?
    if ((buttonPressed && isMenuB) || (!buttonPressed && !isMenuB)) {
        //if (_lockedOut) {
        if (!_rawValStored) {
            // store current raw position
            _rawValStored = true;
            _rawValDiff = smoothVal;
            
        }else if (abs(smoothVal - _rawValDiff) > _quarterStep && _lockedOut) {
            if (isMenuB && !interfaceLockOut) interfaceLockOut = true;
            ledOverRide = true;
            _overRideStatus = true;
            _rawValDiff = smoothVal;
            _overRideTimer = 0;
        }else if (isMenuB) {
            _overRideStatus = false;
        }

        if (_lockedOut && value != _lockValue && _overRideStatus) {
            // pot has moved via other menu
            // override ledbar and show way to turn
            int difference = value - _value;
            if (difference > 0) {
                ledString = 12;
                
            }else if (difference < 0) {
                ledString = 3;
            }
            
        }else if (_lockedOut && value == _lockValue && _overRideStatus) {
            // ledbar is solid fill 
            ledString = 15;
            
            //Serial.println("pot unlocked");
            //Serial.println(value);
            _lockedOut = false;
        }
        //}
        if (value != _value && !_lockedOut) {
            /*
            if (isMenuB) Serial.print("BBB ");
            else Serial.print("AAA ");
            Serial.println(value*_coeff);
            Serial.println(_value);
            Serial.println(value);
            */
            
            _value = value;
            _lockValue = value;
            quantVal = _value * _coeff;
            changed = true;
        }
        
    }else {
        // button is down
        // the other menu of the pot is being used
        _lockedOut = true;
        _rawValStored = false;
    }
}
void Input::update() {
    changed = false;

    // read in value
    rawVal = analogRead(_pin);
    smoothVal = _rollingAvg(rawVal);

    // Increase resolution for 1v/oct and small step sizes
    if (_isHighRes) {
        smoothVal <<= 2; // multiply by 4
    }

    value = _updatePosition();

    if (value != _value) {
        _value = value;
        quantVal = _value * _coeff;
        changed = true;
    }
}

void Input::setVOctCal(int vOctCal) {
    _vOctCal = vOctCal;
}

void Input::setup(int numOfSteps, int percentHyst, int ADC_MAX_VAL) {
    if (numOfSteps > 10) _isHighRes = true;
    
    if (_isHighRes) {
        _stepSize = (float)((ADC_MAX_VAL / (numOfSteps - 1)) * 4) - _vOctCal;
        _coeff = (float) 1 / (((ADC_MAX_VAL / (numOfSteps - 1)) * 4) - _vOctCal);
    }else {
        _stepSize = (float)ADC_MAX_VAL / (numOfSteps - 1);
        _coeff = (float) (numOfSteps - 1) / ADC_MAX_VAL;
    }
    _hysteresis = _stepSize * ((float) percentHyst / 100);
    _quarterStep = _stepSize * .25;
}

int Input::_rollingAvg(int rawValue) {

    // Remove value from sum
    // Update list with new value
    // Update sum
    _avgSum -= _avgPot[_avgCounter];
    _avgPot[_avgCounter] = rawValue;
    _avgSum += _avgPot[_avgCounter];

    // First 128 readings are incorrect, but happens on boot, so ok
    // Update counter and roll over if greater than list size
    _avgCounter++;
    _avgCounter = _avgCounter%128;
    
    return _avgSum >> 7; //divide by 128
}

int Input::_updatePosition() {
    int posValue = _value;
    int ret = _value;
    
    if (smoothVal > posValue + _hysteresis) { // going up
        posValue += _stepSize;
    }else if (smoothVal < posValue - _hysteresis) { // going down
        posValue -= _stepSize;
    }

    if (posValue != _value && smoothVal > 0) {
        //smoothVal = posValue;
        ret = posValue;
    }

    return ret;
}

void Input::printStatus() {
    Serial.print("changed ");
    Serial.println(changed);
    Serial.print("quantVal ");
    Serial.println(quantVal);
    Serial.print("value ");
    Serial.println(value);
    Serial.print("_value ");
    Serial.println(_value);
    Serial.print("smoothVal ");
    Serial.println(smoothVal);
    Serial.print("rawVal ");
    Serial.println(rawVal);

    Serial.println();
    Serial.print("_avgSum ");
    Serial.println(_avgSum);
    Serial.print("_avgCounter ");
    Serial.println(_avgCounter);
    Serial.println();
}

void Input::printSetup() {
    Serial.print("_pin ");
    Serial.println(_pin);
    Serial.print("isMenuB ");
    Serial.println(isMenuB);
    Serial.print("isHighRes ");
    Serial.println(_isHighRes);

    Serial.println();
    Serial.print("_vOctCal ");
    Serial.println(_vOctCal);
    Serial.println();
    Serial.print("_stepSize ");
    Serial.println(_stepSize);
    Serial.print("_coeff * 10k ");
    Serial.println(_coeff*10000);
    Serial.print("_hysteresis ");
    Serial.println(_hysteresis);
    Serial.println();
}


//--------------------
// Define a Button
//--------------------

Button::Button(int pin, int shortPressDuration, int longPressDuration) {
    _pin = pin;
    _shortPressDuration = shortPressDuration;
    _longPressDuration = longPressDuration;
}
/*
void Button::setButtonState(int buttonState, Interface& interface) {
    //Serial.print("buttonState ");
    Serial.println(buttonState);
    //interface._buttonState = buttonState;
}
*/

void Button::update() {
    unsigned int buttonValue = digitalRead(_pin);
    _debounce(buttonValue);

    interfaceButtonState = buttonValue;

    if (_buttonTimer > _longPressDuration && !_buttonState && !interfaceLockOut) {
        longPress = true;
        _buttonTimer = 0;
        
    }else if (_buttonTimer > _shortPressDuration && !_buttonState && !interfaceLockOut) {
        shortPress = true;
        _buttonTimer = 0;

    }else if (!_buttonState && interfaceLockOut) {
        //Serial.println("lockout off");
        interfaceLockOut = false;
        ledOverRide = false;
        _buttonTimer = 0;
        
    }else if (!_buttonState){
        longPress = false;
        shortPress = false;
    }


    // Clear timer after all checks made
    _buttonTimer = _buttonTimer * _buttonState;
}

void Button::_debounce(unsigned int input) {
    // http://www.kennethkuhn.com/electronics/debounce.c

    //unsigned int input;       // 0 or 1 depending on the input signal 
    //unsigned int integrator;  // Will range from 0 to the specified MAXIMUM 
    //unsigned int output = 0;      // Cleaned-up version of the input signal

   // Step 1: Update the integrator based on the input signal.  Note that the 
   // integrator follows the input, decreasing or increasing towards the limits as 
   // determined by the input state (0 or 1). 
    
    if (input == 0)
    {
        if (_integrator > 0)
            _integrator--;

            }
    else if (_integrator < INTEGRATOR_MAXIMUM)
        _integrator++;
    
   // Step 2: Update the output state based on the integrator.  Note that the
   // output will only change states if the integrator has reached a limit, either
   // 0 or MAXIMUM.
    
    if (_integrator == 0) {
        //output = 0;
        _buttonState = 0;

    }else if (_integrator >= INTEGRATOR_MAXIMUM)
    {
        //output = 1;
        _buttonState = 1;
        _integrator = INTEGRATOR_MAXIMUM;  // defensive code if integrator got corrupted
    }

}

//void Button::setLockOut() {
//    _lockOut = true;
//}




/*                    
LedBar::LedBar(int pin0, int pin1, int pin2, int pin3) {
  _pin[0] = pin0;
  _pin[1] = pin1;
  _pin[2] = pin2;
  _pin[3] = pin3;
  _waveform = 0;
  _blinkLength = 500;
  _dutyCycle = 10;
}

bool LedBar::update() {
  int update = 1;

  //blink waveform
  blinkWaveform();
  //dim position
  //  dimPosition();
  //scanner = 1;
  //larsonRun();

  return update;
}
void LedBar::blinkWaveform() {
  // IMPORTANT: this needs to run after
  // dimPosition
  if (_blinkTimer > _blinkLength) {
    _blinkTimer = 0;
    
    if(_blinkStatus) {
      //writeLeds(0);
      Serial.println("wave off");
      _blinkStatus = 0;
    }else {
      //writeLeds(_waveform);
      Serial.print("waveform led on");
      Serial.println(_waveform);
      //_waveformBlink = _waveform;
      _blinkStatus = 1;
    }
  }
}

void LedBar::dimPosition() {
  short position;
  _dutyClock++;

  if (_blinkStatus) position = _pattern | _waveform;
  else position = _pattern;

  //dim with a duty cycle
  if (_dutyClock > _dutyCycle) {
    if (_dimStatus) {
      _dimStatus = 0;
      _dutyClock = 0;
    }else {
      _dimStatus = 1;
      _dutyClock = 9;
    }
  }
  if (_dimStatus) {
    //leds on
    Serial.print(_pattern);
    Serial.print(" -- xpattern ");
    Serial.println(_xPattern);
    
    writeLeds(position);
  }else {
    if (_blinkStatus) writeLeds(_waveform);
    else writeLeds(0);
  }
}

void LedBar::startup() {
  // We were just initialized. Ignore everything and larsen scan
  // for a few seconds.

  //  larsonScanStart(10000);
}

void LedBar::ledOn(int pin) {
  ledAction(pin, 1);
}

void LedBar::ledOff(int pin) {
  ledAction(pin, 0);
}

void LedBar::ledAction(int pin, bool status) {
  digitalWrite(_pin[pin], status);
}

void LedBar::writeLeds(short pattern) {
  //
  //_xPattern = (16 ^ _waveform) & pattern;
  //_pattern = pattern;
  //Serial.print(_waveform);
  //Serial.print(" pattern:");
  //Serial.println(pattern);
  //pattern |= _waveform;
  
  ledAction(0, pattern & 0b1000);
  ledAction(1, pattern & 0b0100);
  ledAction(2, pattern & 0b0010);
  ledAction(3, pattern & 0b0001);
}

void LedBar::writeStatus16(short pattern) {
  //writeLeds(status16[pattern]);
  _pattern = status16[pattern];
  _xPattern = (~_waveform & 0b1111) & _pattern;
}

void LedBar::setWaveform(short pattern) {
  _waveform = pattern;
}

void LedBar::larsonScanStart(int timeToScan) {
  if (!scanner) {
    scanTime = timeToScan;
    scannerTimer = 0;
    scannerCount = 0;
    scanner = 1;
  }else if (scannerTimer > scanTime) {
    scanner = 0;
  }
  
  larsonScan();
}

void LedBar::larsonScanStop() {
  scanner = 0;
  larsonScan();
}

void LedBar::larsonScan() {
  
  if (scannerTimer > scanTime || !scanner) {
    scanner = 0;
    writeLeds(0b0000);
  }else {
    larsonRun();
  }
}

void LedBar::larsonRun() {
  unsigned int delay = 100;
  
  if (scanner && scannerClock > delay) {
    writeLeds(larsonPat[scannerCount%14]);
    scannerCount++;
    scannerClock = 0;
  }
}
    
*/
