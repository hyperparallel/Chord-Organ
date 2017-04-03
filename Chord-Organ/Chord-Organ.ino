#include <Bounce2.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <EEPROM.h>

#include "Settings.h"
#include "Waves.h"
#include "Interface.h"

// #define DEBUG_STARTUP
// #define DEBUG_MODE
// #define CHECK_CPU

#define CHORD_POT_PIN 9 // pin for Chord pot
#define CHORD_CV_PIN 6 // pin for Chord CV 
#define ROOT_POT_PIN 7 // pin for Root Note pot
#define ROOT_CV_PIN 8 // pin for Root Note CV
#define RESET_BUTTON 8 // Reset button 
#define RESET_LED 11 // Reset LED indicator 
#define RESET_CV 9 // Reset pulse in / out
#define BANK_BUTTON 2 // Bank Button 
#define LED0 6
#define LED1 5
#define LED2 4
#define LED3 3

// REBOOT CODES 
#define RESTART_ADDR       0xE000ED0C
#define READ_RESTART()     (*(volatile uint32_t *)RESTART_ADDR)
#define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))

#define ADC_BITS 13
#define ADC_MAX_VAL 8192
#define CHANGE_TOLERANCE 64

#define SINECOUNT 8
#define LOW_NOTE 36
#define V_OCT_NOTE 36

// For arbitrary waveform, required but unused apparently.
#define MAX_FREQ 600

#define SHORT_PRESS_DURATION 10
#define LONG_PRESS_DURATION 1000

#define ROOT_NOTES_NUM 48

// Define modes
#define CHORD_MODE 0
#define ARP_MODE 1


LedBar leds(LED3, LED2, LED1, LED0);

InputPot chordPot(CHORD_POT_PIN);
InputPot chordPotB(CHORD_POT_PIN);

InputPot rootPot(ROOT_POT_PIN);
InputPot rootPotB(ROOT_POT_PIN);

Input chordCV(CHORD_CV_PIN);
Input rootCV(ROOT_CV_PIN);

Button resetButton(RESET_BUTTON, SHORT_PRESS_DURATION, LONG_PRESS_DURATION);

int chordCount = 16;

int sum[4] = {0,0,0,0};
int avgIndex[4] = {0,0,0,0};
int avgPot[4][128];

// Target frequency of each oscillator
float FREQ[SINECOUNT] = {
    55,110, 220, 440, 880,1760,3520,7040};

// Total distance between last note and new.
// NOT distance per time step.
float deltaFrequency[SINECOUNT] = {
    0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};

// Keep track of current frequency of each oscillator
float currentFrequency[SINECOUNT]  = {
    55,110, 220, 440, 880,1760,3520,7040};

float AMP[SINECOUNT] = { 
    0.9, 0.9, 0.9, 0.9,0.9, 0.9, 0.9, 0.9};

// Volume for a single voice for each chord size
float AMP_PER_VOICE[SINECOUNT] = {
  0.4,0.3,0.22,0.2,0.15,0.15,0.13,0.12};

// Store midi note number to frequency in a table
// Later can replace the table for custom tunings / scala support.
float MIDI_TO_FREQ[128];

int chordRaw;
int chordRawOld;
int chordCVRaw;
int chordCVRawOld;
int chordQuant;
int chordQuantChk;
int chordQuantOld;

int chordBounce = 0;
int chordCVBounce = 0;
int rootBounce = 0;
int rootCVBounce = 0;

int voiceQuant = 0;
int voiceQuantOld = 0;;

int rootOld;
int rootCVOld;

int rootQuant;
int rootCVQuantOld;
int rootQuantOld;

int chordStep;

int chordHysteresis;

int chordCVStep;
int chordCVHysteresis;

int rootStep;
int rootHysteresis;

int rootCVStep;
int rootCVHysteresis;

float chordCoeff;
float chordCVCoeff;
float rootCoeff;
float rootCVCoeff;

float rootMapCoeff;

// Root CV Pin readings below this level are clamped to LOW_NOTE
int rootClampLow;

// Flag for either chord or root note change
boolean changed = true;
boolean chordChanged = false;
boolean chordCVChanged = false;
boolean rootChanged = false;
boolean rootCVChanged = false;

Bounce resetCV = Bounce( RESET_CV, 40 ); 
//boolean resetButton = false;
boolean resetCVRose;

elapsedMillis resetHold;
elapsedMillis resetFlash; 
int updateCount = 0;

elapsedMillis buttonTimer = 0;
elapsedMillis lockOut = 0;
boolean shortPress = false;
boolean longPress = false;
elapsedMillis pulseOutTimer = 0;
uint32_t flashTime = 10;
boolean flashing = false;

// WAVEFORM
// Default wave types
short wave_type[4] = {
    WAVEFORM_SINE,
    WAVEFORM_SQUARE,
    WAVEFORM_SAWTOOTH,
    WAVEFORM_PULSE,
};
// Current waveform index
int waveform = 0;

// Made voiceCount global so the arp can use it
int voiceCount = 0;

// Waveform LED
boolean flashingWave = false;
elapsedMillis waveformIndicatorTimer = 0;

// Translate for leds
short trans[4] = {8, 4, 2, 1};
//bool ledOverRide = false;

int waveformPage = 0;
int waveformPages = 1;

// Custom wavetables
int16_t const* waveTables[8] {
    wave1,
    wave7,
    wave3,
    wave4,    

    wave8,
    wave9,
    wave10,
    wave11
};

// Per-waveform amp level
// First 4 are default waves, last 8 are custom wavetables
float WAVEFORM_AMP[12] = {
  0.8,0.6,0.8,0.6,
  0.8,0.8,0.8,0.8,
  0.8,0.8,0.8,0.8,
};

// GLIDE
// Main flag for glide on / off
boolean glide = false;
// msecs glide time. 
uint32_t glideTime = 50;
// keep reciprocal
float oneOverGlideTime = 0.02;
// Time since glide started
elapsedMillis glideTimer = 0;
// Are we currently gliding notes
boolean gliding = false;
// Voicing flag
boolean useVoicing = false;
// Enable 1v/Oct
boolean vOct = false;

// Stack mode replicates first 4 voices into last 4 with tuning offset
boolean stacked = false;
float stackFreqScale = 1.001;

// Select CV control for voicing
int cvSelect = 0;

int noteRange = 38;
int vOctCal = 48;

// Arp
int mode = 0;
int arpCounter = 0;
int arpClock = 0;
int arpClockBounce = 0;
int arpClockThresh = 10; //num ticks low to stop counting pulse in
int arpPulseArmed = 0; //only let trigger fire once per count
int arpDirection = 1;
int arpNumModes = 5; //count from 0
int arpVal = 0;

// GUItool: begin automatically generated code

AudioSynthWaveform       waveform1;      //xy=215,232
AudioSynthWaveform       waveform2;      //xy=243,295
AudioSynthWaveform       waveform3;      //xy=273,354
AudioSynthWaveform       waveform4;      //xy=292,394
AudioSynthWaveform       waveform5;      //xy=215,232
AudioSynthWaveform       waveform6;      //xy=243,295
AudioSynthWaveform       waveform7;      //xy=273,354
AudioSynthWaveform       waveform8;      //xy=292,394
AudioMixer4              mixer1;         //xy=424,117
AudioMixer4              mixer2;         //xy=424,181
AudioMixer4              mixer3;         //xy=571,84
AudioEffectEnvelope      envelope1;      //xy=652,281
AudioOutputAnalog        dac1;           //xy=784,129
AudioConnection          patchCord1(waveform1, 0, mixer1, 0);
AudioConnection          patchCord2(waveform2, 0, mixer1, 1);
AudioConnection          patchCord7(waveform3, 0, mixer1, 2);
AudioConnection          patchCord8(waveform4, 0, mixer1, 3);
AudioConnection          patchCord3(waveform5, 0, mixer2, 0);
AudioConnection          patchCord4(waveform6, 0, mixer2, 1);
AudioConnection          patchCord5(waveform7, 0, mixer2, 2);
AudioConnection          patchCord6(waveform8, 0, mixer2, 3);
AudioConnection          patchCord9(mixer1, 0, mixer3, 0);
AudioConnection          patchCord10(mixer2, 0, mixer3, 1);
AudioConnection          patchCord11(mixer3, envelope1);
AudioConnection          patchCord12(envelope1, dac1);
// GUItool: end automatically generated code
// Pointers to waveforms
AudioSynthWaveform* oscillator[8];

Settings settings("CHORDORG.TXT");

void setup(){
    pinMode(BANK_BUTTON,INPUT);
    pinMode(RESET_BUTTON, INPUT);
    pinMode(RESET_CV, INPUT); 
    pinMode(RESET_LED, OUTPUT);
    pinMode(LED0,OUTPUT);
    pinMode(LED1,OUTPUT);
    pinMode(LED2,OUTPUT);
    pinMode(LED3,OUTPUT);
    AudioMemory(50);
    analogReadRes(ADC_BITS);
    
    oscillator[0] = &waveform1;
    oscillator[1] = &waveform2;
    oscillator[2] = &waveform3;
    oscillator[3] = &waveform4;
    oscillator[4] = &waveform5;
    oscillator[5] = &waveform6;
    oscillator[6] = &waveform7;
    oscillator[7] = &waveform8;

    for(int i=0;i<128;i++) {
        MIDI_TO_FREQ[i] = numToFreq(i);
    }

#ifdef DEBUG_STARTUP
  while( !Serial );

    Serial.println("Starting");
    // ledWrite(waveform);
#endif // DEBUG_STARTUP

    // SD CARD SETTINGS FOR MODULE 
    SPI.setMOSI(7);
    SPI.setSCK(14);

    // Read waveform settings from EEPROM 
    waveform = EEPROM.read(1234);

#ifdef DEBUG_STARTUP
    Serial.print("Waveform from EEPROM ");
    Serial.println(waveform);
#endif

    if (waveform < 0) waveform = 0;
    ledWrite(waveform % 4);

    // OPEN SD CARD 
    boolean hasSD = openSDCard();

#ifdef DEBUG_STARTUP
    Serial.print("Has SD ");
    Serial.println(hasSD);
#endif    
    // READ SETTINGS FROM SD CARD 
    settings.init(hasSD);

    chordCount = settings.numChords;
    waveformPages = settings.extraWaves ? 3 : 1;
    if(waveformPages > 1) {
        waveformPage = waveform >> 2;
    } else {
        // If we read a custom waveform index from EEPROM
        // but they are not enabled in the config then change back to sine
        waveform = 0;
    }

    glide = settings.glide;
    glideTime = settings.glideTime;
    oneOverGlideTime = 1.0 / (float) glideTime;
    noteRange = settings.noteRange;
    stacked = settings.stacked;
    useVoicing = settings.useVoicing;
    //cvSelect = settings.cvSelect;
    vOct = settings.vOct;
    vOctCal = settings.vOctCal;
    
#ifdef DEBUG_STARTUP
    Serial.print("Waveform page ");
    Serial.println(waveformPage);
    Serial.print("Waveform set to ");
    Serial.println(waveform);

    Serial.println("-- Settings --");
    Serial.print("Chord Count ");
    Serial.println(chordCount);
    Serial.print("Waveform Pages ");
    Serial.println(waveformPages);
    Serial.print("Glide ");
    Serial.println(glide);
    Serial.print("Glide Time ");
    Serial.println(glideTime);
    Serial.print("Note Range ");
    Serial.println(noteRange);
    Serial.print("Stacked ");
    Serial.println(stacked);

#endif

    // Setup audio
    for(int i=0;i<SINECOUNT;i++) {
        oscillator[i]->pulseWidth(0.5);
    }

    for(int m=0;m<4;m++) {
        mixer1.gain(m,0.25);
        mixer2.gain(m,0.25);
    }

    mixer3.gain(0,0.49);
    mixer3.gain(1,0.49);
    mixer3.gain(2,0);
    mixer3.gain(3,0);

    envelope1.attack(1);
    envelope1.decay(1);
    envelope1.sustain(1.0);
    envelope1.release(1);
    envelope1.noteOn();

    if(waveformPage == 0) {
        // First page is built in waveforms
        setWaveformType(wave_type[waveform]);
    } else {
        // Second and third pages are arbitrary waves
        setupCustomWaveform(waveform);
        // Start the wave led flashing
        flashingWave = true;
        waveformIndicatorTimer = 0;
    }

    
    // This allows us to fine tune the 1v/oct scaling
    //  default vOctCal (calibration) is 48
    //  this equals previous default of 38 notes
    //  
    //  Note: 36   37   38   39   40   41   42   43   44   45
    //  Cal:  0    25   48   70   91   111  130  148  166  182

    //int voctRange = ADC_MAX_VAL/36 - voctCalibration;
    
    chordPotB.isMenuB = true;
    rootPotB.isMenuB = true;
    
    chordPot.setup(chordCount, 70, ADC_MAX_VAL);
    chordPotB.setup(chordCount, 70, ADC_MAX_VAL);

    chordCV.setup(chordCount, 55, ADC_MAX_VAL, true);
    //chordCV.setIsCV(true);
    rootPot.setup(ROOT_NOTES_NUM, 70, ADC_MAX_VAL);
    rootPotB.setup(3, 70, ADC_MAX_VAL);  // chord cv changes 0=chord, 1=voicing, 2=dual

    if (vOct) {
        //rootCV.setVOctCal(vOctCal);
        rootCV.setVOctCal(104);
        rootCV.setup(V_OCT_NOTE, 55, ADC_MAX_VAL, true);
    }else {
        rootCV.setup(noteRange, 55, ADC_MAX_VAL, true);
    }
    //rootCV.setIsCV(true);
    /*   
    delay(2000);
    int CVtest = rootCVCoeff * 1000000;

    Serial.print("rootCVClampLow x4 ");
    Serial.println(rootCVClampLow);
    Serial.print("rootCVStep /4 ");
    Serial.println(rootCVStep);
    Serial.print("rootCVCoeff x4 ");
    Serial.println(CVtest);
    Serial.print("rootCVHysteresis x4 ");
    Serial.println(rootCVHysteresis);
    */
    // seed randoms for arp
    randomSeed(analogRead(5));

#ifdef DEBUG_STARTUP
    Serial.print("Root Clamp Low ");
    Serial.println(rootClampLow);
    Serial.print("Root Map Coeff ");
    Serial.println(rootMapCoeff * 100);
#endif

}

boolean openSDCard() {
    int crashCountdown = 0; 
    if (!(SD.begin(10))) {
        while (!(SD.begin(10))) {
            ledWrite(15);
            delay(20);
            ledWrite(crashCountdown % 4);
            delay(20);
            crashCountdown++;
            if (crashCountdown > 4) {
                return false;
            }
        }
    }
    return true;
}

void loop(){
  
    checkInterface();

    if (changed) {

        // Serial.println("Changed");
        updateAmpAndFreq();
        if(glide) {
            glideTimer = 0;
            gliding = true;
            // Serial.println("Start glide");
        }

        #ifdef CHECK_CPU
        int maxCPU = AudioProcessorUsageMax();
        Serial.print("MaxCPU=");
        Serial.println(maxCPU);
        #endif // CHECK_CPU
    }

    // CHECK BUTTON STATUS 
    //resetHold = resetHold * resetButton;

    if (resetButton.shortPress){
        Serial.println("ShortPress");
        waveform++;
        waveform = waveform % (4 * waveformPages);
        selectWaveform(waveform);
        changed = true;
        //shortPress = false;
    }

    if (resetButton.longPress) {
        mode++;
        mode %= 2;
        changed = true;
        Serial.println("longPress");

        if (mode == ARP_MODE) {
            stacked = false;
            // glide could work with a few changes setting up delta freq
            // problem is getting correct chord[i] in  updateFrequencies()
            glide = false;
            rootPotB.setup(5, 70, ADC_MAX_VAL); // number of arp modes
            //rootPotB.quantVal = 0; // this should probably be protected or in setup
        }else {
            stacked = settings.stacked;
            glide = settings.glide;
            rootPotB.setup(3, 70, ADC_MAX_VAL);
            //rootPotB.quantVal = 0;
        }
    }

    if (changed)  {

        if (mode == ARP_MODE && arpCounter > 0) {
          flashing = false;
          arpPulseArmed = 0;
        }else if (!arpPulseArmed || mode != ARP_MODE) {
          flashing = true;
          pulseOutTimer = 0;        
          pinMode(RESET_CV, OUTPUT);
          digitalWrite (RESET_LED, HIGH);
          digitalWrite (RESET_CV, HIGH);
        }



        /*
        // Serial.println("Trig Out");
        pulseOutTimer = 0;
        flashing = true;
        pinMode(RESET_CV, OUTPUT);
        digitalWrite (RESET_LED, HIGH);
        digitalWrite (RESET_CV, HIGH);
        */
        AudioNoInterrupts();
        updateFrequencies();
        updateAmps();
        AudioInterrupts();

        changed = false;
    }

    if(gliding) {
        if(glideTimer >= glideTime) {
            gliding = false;
        }
        AudioNoInterrupts();
        updateFrequencies();
        AudioInterrupts();
    }

    updateWaveformLEDs();

    if (pulseOutTimer > flashTime && flashing) {
        arpPulseArmed = 1;
        digitalWrite (RESET_LED, LOW);
        digitalWrite (RESET_CV, LOW);
        pinMode(RESET_CV, INPUT);
        flashing = false;  
    } 
/*
    if (flashing && (pulseOutTimer > flashTime)) {
        digitalWrite (RESET_LED, LOW);
        digitalWrite (RESET_CV, LOW);
        pinMode(RESET_CV, INPUT);
        flashing = false;  
    } 
*/
}

// Calculate hysteresis and step size of inputs
int indexStep (int range) {
  return (float)ADC_MAX_VAL / (range - 1);
}

void updateAmpAndFreq() {
    int16_t* chord = settings.notes[chordQuant];
    int16_t* voicing = settings.voicings[voiceQuant];

    int noteNumber;
    int halfSinecount = SINECOUNT>>1;
    voiceCount = 0;


    if(stacked) {
        for(int i=0;i < halfSinecount;i++) {
            if (chord[i] != 255) {
                noteNumber = rootQuant + chord[i];
                noteNumber += voicing[i];
                if(noteNumber < 0) noteNumber = 0;
                if(noteNumber > 127) noteNumber = 127;
                float newFreq = MIDI_TO_FREQ[noteNumber];

                FREQ[i] = newFreq;
                FREQ[i+halfSinecount] = newFreq * stackFreqScale;
                // Serial.println("Stack Freq");
                // Serial.println(FREQ[i]);
                // Serial.println(FREQ[i+halfSinecount]);

                deltaFrequency[i] = newFreq - currentFrequency[i];
                deltaFrequency[i+halfSinecount] = (newFreq * stackFreqScale) - currentFrequency[i];

                voiceCount += 2;
            }            
        }
    } else {
        for(int i = 0; i< SINECOUNT; i++){
            if (chord[i] != 255) {
                noteNumber = rootQuant + chord[i];
                noteNumber += voicing[i];
                if(noteNumber < 0) noteNumber = 0;
                if(noteNumber > 127) noteNumber = 127;
                float newFreq = MIDI_TO_FREQ[noteNumber];

                // TODO : Allow option to choose between jump from current or new?
                //deltaFrequency[i] = newFreq - FREQ[i];
                deltaFrequency[i] = newFreq - currentFrequency[i];

                // Serial.print("Delta ");
                // Serial.print(i);
                // Serial.print(" ");
                // Serial.print(deltaFrequency[i]);
                // Serial.print(" ");
                // Serial.println(newFreq);

                FREQ[i] = newFreq;
                if (i == arpCounter && mode == ARP_MODE) FREQ[0] = FREQ[i];
                voiceCount++;
            }
        }

    }

    float ampPerVoice = AMP_PER_VOICE[voiceCount-1];
    float totalAmp = 0;

    if(stacked) {
        for (int i = 0; i < halfSinecount; i++){
            if (chord[i] != 255) {
                AMP[i] = ampPerVoice;
                AMP[i + halfSinecount] = ampPerVoice; 
                totalAmp += ampPerVoice;
            }
            else{
                AMP[i] = 0.0;   
            }
        }        
    } else {
        for (int i = 0; i< SINECOUNT; i++){
            if (chord[i] != 255) {
                AMP[i] = ampPerVoice;
                totalAmp += ampPerVoice;
            }
            else{
                AMP[i] = 0.0;   
            }
        }        
    }
}

void selectWaveform(int waveform) {
    waveformPage = waveform >> 2;
    if(waveformPage > 0) {
        flashingWave = true;
        waveformIndicatorTimer = 0;
    }  
    ledWrite(waveform % 4);
    EEPROM.write(1234, waveform);

    #ifdef DEBUG_MODE
    Serial.print("Waveform ");
    Serial.println(waveform);
    Serial.print("Waveform page ");
    Serial.println(waveformPage);
    #endif // DEBUG_MODE

    AudioNoInterrupts();
    if(waveformPage == 0) {
        setWaveformType(wave_type[waveform]);
    } else {
        setupCustomWaveform(waveform);    
    }
    AudioInterrupts();    
}

void setWaveformType(short waveformType) {
    for(int i=0;i<SINECOUNT;i++) {
        oscillator[i]->begin(1.0,FREQ[i],waveformType);
    }   
}

void setupCustomWaveform(int waveselect) {
    waveselect = (waveselect - 4) % 8;

    const int16_t* wave = waveTables[waveselect];
    for(int i=0;i<SINECOUNT;i++) {
        oscillator[i]->arbitraryWaveform(wave, MAX_FREQ);
    }

    setWaveformType(WAVEFORM_ARBITRARY);
}

void updateWaveformLEDs() {
    // Flash waveform LEDs for custom waves
    if(waveformPage > 0) {
        uint32_t blinkTime = 100 + ((waveformPage - 1) * 300);
        if(waveformIndicatorTimer >= blinkTime) {
            waveformIndicatorTimer = 0;
            flashingWave = !flashingWave;
            if(flashingWave) {
                ledWrite(waveform % 4);
            } else {
                ledWrite(15);
            }
        }
    }    
}

void updateFrequencies() {

    if(gliding) {
        // TODO : Replace division with reciprocal multiply.
        float dt = 1.0 - (glideTimer * oneOverGlideTime);
        if(dt < 0.0) {
            dt = 0.0;
            gliding = false;
        }
        // Serial.print("dt ");
        // Serial.print(dt);
        // Serial.print(" ");
        // Serial.println(glideTimer);

        for(int i=0;i<SINECOUNT;i++) {
            currentFrequency[i] = FREQ[i] - (deltaFrequency[i] * dt);
            oscillator[i]->frequency(currentFrequency[i]);
        }
    } else {
        for(int i=0;i<SINECOUNT;i++) {
            oscillator[i]->frequency(FREQ[i]);
        }
    }
}

void updateAmps(){
        float waveAmp = WAVEFORM_AMP[waveform];
        mixer1.gain(0,AMP[0] * waveAmp);
        
        if (mode == ARP_MODE) waveAmp = 0;
        mixer1.gain(1,AMP[1] * waveAmp);
        mixer1.gain(2,AMP[2] * waveAmp);
        mixer1.gain(3,AMP[3] * waveAmp);
        mixer2.gain(0,AMP[4] * waveAmp);
        mixer2.gain(1,AMP[5] * waveAmp);
        mixer2.gain(2,AMP[6] * waveAmp);
        mixer2.gain(3,AMP[7] * waveAmp);
}

// WRITE A 4 DIGIT BINARY NUMBER TO LED0-LED3 
void ledWrite(int n){
    if (n > 3) {
        leds.ledWriteBit(0);
    } else {
        leds.ledWriteBit(trans[n]);
    }
}


void checkInterface(){

    int rootCVQuant = LOW_NOTE;
    
    chordChanged = false;
    chordCVChanged = false;
    rootChanged = false;
    rootCVChanged = false;

    chordPot.update();
    chordPotB.update();
    chordCV.update();
    rootPot.update();
    rootPotB.update();
    rootCV.update();
    leds.update();
    
    chordChanged = chordPot.changed;
    chordCVChanged = chordCV.changed;
    rootChanged = rootPot.changed;
    rootCVChanged = rootCV.changed;


    //if (arpMode) {
    // NEEDS
    // arpVal
    // arpCounter
    // arpDirection
    
    if (mode == ARP_MODE) {
        
        arpVal = rootPotB.quantVal;
        /*
          chordRaw = chordPot;
          chordCvRaw = chordCV;
          chordCvRaw = constrain(chordCvRaw, 0, 1023);
        */
        //if (chordCvRaw > 1020 && !arpClock) { // new clock pulse received
        if (chordCV.rawVal > 8100 && !arpClock) {
            arpClock = 1;
            arpClockBounce = 0;
            changed = true;
        
            switch (arpVal) {
            case 0:  // Arp up
                arpCounter++;
                if (arpCounter == voiceCount) arpCounter = 0;          
                break;
          
            case 1:  // Arp down
                arpCounter--;
                if (arpCounter < 0) arpCounter = voiceCount - 1;
                break;
                
            case 2:  // Ping pong single notes at top and bottom
                arpCounter = arpCounter + arpDirection;
                if (arpCounter == voiceCount) {
                    arpDirection = -1;
                    arpCounter = voiceCount - 2;
                }else if (arpCounter < 0) {
                    arpDirection = 1;
                    arpCounter = 1;
                }
                break;
                
            case 3:  // Ping pong with double trigger and double notes
                arpCounter = arpCounter + arpDirection;
                if (arpCounter == voiceCount) {
                    arpDirection = -1;
                    arpCounter = voiceCount - 1;
                }else if (arpCounter < 0) {
                    arpDirection = 1;
                    arpCounter = 0;
                }
                break;
                
            case 4:  // Random values
                arpCounter = random(voiceCount - 1);
                break;
                
          // also could do 2 up 1 down
          // 3 up 1 down, 3 down 1 up etc
        }          
	
      }else if (chordCV.rawVal < 100 && arpClock) {  // clock pulse ended 
        if (arpClockBounce < arpClockThresh) arpClockBounce++;
        else arpClock = 0;
      }
      
    }

    
    
    // prevent a chord update if list is held at top value
    //if (chordChanged || chordPotB.changed || chordCVChanged || rootPotB.changed) {
    if (chordPot.changed || chordPotB.changed || chordCV.changed || rootPotB.changed) {
        cvSelect = rootPotB.quantVal;
        chordRaw = chordPot.quantVal;
        /*
        if (mode == ARP_MODE) {
            // ignore cv value
            chordCVRaw = (chordPotB.quantVal) % chordCount;
        }else if (mode == CHORD_MODE) {
            chordCVRaw = (chordCV.quantVal + chordPotB.quantVal) % chordCount;
        }
        */      
        if(mode != ARP_MODE) {
            if(cvSelect == 0) {
                chordQuant = constrain(chordPot.quantVal + chordCV.quantVal, 0, chordCount - 1);
                voiceQuant = chordPotB.quantVal;
                //Serial.println(" useVoicing and cvSelect == 0");
            }else if(cvSelect == 1) {
                chordQuant = chordPot.quantVal;
                voiceQuant = (chordPotB.quantVal + chordCV.quantVal) % chordCount;
                //Serial.println(" useVoicing and cvSelect == 1");
            }
        }else {
            chordQuant = chordPot.quantVal;
            voiceQuant = chordPotB.quantVal;
        }

            //chordQuant = constrain(chordRaw + chordCVRaw, 0, chordCount);
        //Serial.println(" useVoicing is False ");
        //}
        
        chordPot.printStatus();
        changed = true;
        /*
          Serial.print("chordQuant: ");
          Serial.print(chordQuant);
          Serial.print("  CV bounce: ");
          Serial.print(chordBounce);
          Serial.print("  ChordPot: ");
          Serial.print(analogRead(CHORD_POT_PIN));
          Serial.print("  ChordCV: ");
          Serial.println(chordCV);
        */
    }
    
    if (rootChanged || rootCVChanged ) {

        rootQuant = rootPot.quantVal + rootCV.quantVal + LOW_NOTE;
        Serial.println("ROOT");
        rootPot.printStatus();      
        
        changed = true;
        //rootCV.printStatus();

        /*
          Serial.print("rootCVBounce ");
          Serial.print(rootCVBounce);
          Serial.print("   rootCV ");
          Serial.println(rootCV);
          Serial.print(analogRead(ROOT_POT_PIN));
          
          Serial.print("rootPot ");
          Serial.print(rootPot);
          Serial.print("  RootCVQuant ");
          Serial.println(rootQuant);
        */
    }
    
#ifdef DEBUG_MODE
   if(rootChanged) {
        // printRootInfo(rootPot,rootCV);
   }
#endif

    //    resetSwitch.update();
    //    resetButton = resetSwitch.read();

   resetButton.update();
/*
    if (!flashing) {
        digitalWrite (RESET_LED, LOW);
        digitalWrite (RESET_CV, LOW);
        pinMode(RESET_CV, INPUT);
    }


   if (!flashing){
      resetCV.update();
      resetCVRose = resetCV.rose();
      if (resetCVRose) resetFlash = 0; 

        digitalWrite(RESET_LED, (resetFlash<20));
    }
*/
}

void reBoot(int delayTime){
    if (delayTime > 0)
        delay (delayTime);
    WRITE_RESTART(0x5FA0004);
}

void printRootInfo(int rootPot, int rootCV) {
    Serial.print("Root ");
    Serial.print(rootPot);
    Serial.print(" ");
    Serial.print(rootCV);
    Serial.print(" ");
    Serial.println(rootQuant);
}

void printPlaying(){
    Serial.print("Chord: ");
    Serial.print(chordQuant);
    Serial.print(" Root: ");
    Serial.print(rootQuant);
    Serial.print(" ");
    for(int i = 0; i<SINECOUNT; i++){
        Serial.print(i);
        Serial.print(": ");
        Serial.print (FREQ[i]);
        Serial.print(" ");
        Serial.print(AMP[i]);
        Serial.print (" | ");
    }
    Serial.println("--");

}

float numToFreq(int input) {
    int number = input - 21; // set to midi note numbers = start with 21 at A0 
    number = number - 48; // A0 is 48 steps below A4 = 440hz
    return 440*(pow (1.059463094359,number));
}
