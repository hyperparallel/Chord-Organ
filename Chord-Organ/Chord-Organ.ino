#include <Bounce2.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <EEPROM.h>

#define CHORD_POT_PIN 9 // pin for Channel pot
#define CHORD_CV_PIN 6 // pin for Channel CV 
#define ROOT_POT_PIN 7 // pin for Time pot
#define ROOT_CV_PIN 8 // pin for Time CV
#define RESET_BUTTON 8 // Reset button 
#define RESET_LED 11 // Reset LED indicator 
#define RESET_CV 9 // Reset pulse input 
#define BANK_BUTTON 2 // Bank Button 
#define LED0 6
#define LED1 5
#define LED2 4
#define LED3 3

// REBOOT CODES 
#define RESTART_ADDR       0xE000ED0C
#define READ_RESTART()     (*(volatile uint32_t *)RESTART_ADDR)
#define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))
#define SINECOUNT 8

//Default spread patterns
int chordSpreadList[16][8] = {
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0}
};

// Initialise Array with 999s, to identify unfilled elements when reading from SD card 
int notesSD[16][8] = {
    {        
        999,999,999,999,999,999,999,999                                                                                                                                                                    }
    ,    
    {        
        999,999,999,999,999,999,999,999                                                                                                                                                                     }
    ,    
    {   
        999,999,999,999,999,999,999,999                                                                                                                                                                     }
    ,    
    {
        999,999,999,999,999,999,999,999                                                                                                                                                                     }
    ,    
    {
        999,999,999,999,999,999,999,999                                                                                                                                                                     }
    ,    
    {
        999,999,999,999,999,999,999,999                                                                                                                                                                     }
    ,    
    {
        999,999,999,999,999,999,999,999                                                                                                                                                                     }
    ,    
    {
        999,999,999,999,999,999,999,999                                                                                                                                                                     }
    ,    
    {
        999,999,999,999,999,999,999,999                                                                                                                                                                     }
    ,    
    {
        999,999,999,999,999,999,999,999                                                                                                                                                                     }
    ,    
    {
        999,999,999,999,999,999,999,999                                                                                                                                                                     }
    ,    
    {
        999,999,999,999,999,999,999,999                                                                                                                                                                     }
    ,    
    {
        999,999,999,999,999,999,999,999                                                                                                                                                                     }
    ,    
    {
        999,999,999,999,999,999,999,999                                                                                                                                                                     }
    ,    
    {
        999,999,999,999,999,999,999,999                                                                                                                                                                     }
    ,    
    {
        999,999,999,999,999,999,999,999                                                                                                                                                                    }
    ,    
};


//Bounce resetCV = Bounce( RESET_CV, 40 ); 
boolean resetButton = false;
File root;
File settingsFile;
int chordCount = 16;
short wave_type[4] = {
    WAVEFORM_SINE,
    WAVEFORM_SQUARE,
    WAVEFORM_SAWTOOTH,
    WAVEFORM_PULSE,
};
int waveform = 0; 
float FREQ[SINECOUNT] = {
    55,110, 220, 440, 880,1760,3520,7040};
float AMP[SINECOUNT] = { 
    0.9, 0.9, 0.9, 0.9,0.9, 0.9, 0.9, 0.9};
float voiceCount = 0;
//int startNote; 
//int chordPick; 
//float startChosen;
//float chordChosen;
//float startOld;
//float chordOld;

int chordRaw;
int chordRawOld;
//int chordQuant;
int potA = 0;
int potB = 0;
int chordQuant = 0; // put in default to test spreading
int chordQuantOld;
int chordSpread = 0;
int chordSpreadOld;
int chordSpreadPot = 0;
int chordSpreadPotOld;
int chordCvInput = 0; //specifies what chord cv controls
int rootRaw;
int rootRawOld;
int rootPotOld;
int rootQuant;
int rootQuantOld;
int rootQuantStore;
int rootMap[1024];
int rootControlOld;
int chordCvRaw;
int chordCvRawOld;
int chordRawStore;
int chordQuantStore;
float arpFreq;
int arpCounter = 0;
int arpClock = 0;
int arpClockBounce = 0;
int arpClockThresh = 10; //num ticks low to stop counting pulse in
int arpPulseArmed = 0; //only let trigger fire once per count
int arpDirection = 1;
int arpNumModes = 5; //count from 0
int arpMode = 0;
boolean changed = true;

//button
int buttonState = 0;

//menuB stuff
boolean menuB = false;
boolean potTurnA = false;
boolean potTurnB = false;
boolean potLockA = false;
boolean potLockB = false;
int potARaw;
int potBRaw;

boolean ResetCV;
//elapsedMillis resetHold;
//elapsedMillis resetFlash; 
int updateCount = 0;

elapsedMillis elapsed1 = 0;
elapsedMillis lockOut = 0;
boolean shortPress = false;
boolean longPress = false;
elapsedMillis pulseOut = 0;
int flashTime = 20; // was 10
boolean flashing = false;

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
//AudioEffectEnvelope      envelope1;      //xy=652,281
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
//AudioConnection          patchCord11(mixer3, envelope1);
//AudioConnection          patchCord12(envelope1, dac1);
AudioConnection          patchCord12(mixer3, dac1);
// GUItool: end automatically generated code


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

    //     delay(2000);
    //     Serial.begin(9600);

    Serial.println("starting");
    ledWrite(waveform);

    // SD CARD SETTINGS FOR MODULE 
    SPI.setMOSI(7);
    SPI.setSCK(14);

    // Read waveform settings from EEPROM 
    waveform = EEPROM.read(1234);
    if (waveform > 3 || waveform < 0) waveform = 0;
    ledWrite(waveform);

    // OPEN SD CARD 
    int crashCountdown = 0; 
    if (!(SD.begin(10))) {
        while (!(SD.begin(10))) {
            ledWrite(15);
            delay(100);
            ledWrite(0);
            delay(100);
            crashCountdown++;
            if (crashCountdown > 6)     
                reBoot(500);
        }
    }
    // READ SETTINGS FROM SD CARD 
    root = SD.open("/");  
    if (SD.exists("CHORDHYP.TXT")) {
        //OVERWRITE STARTUP FILE
        //writeSDSettings();
        readSDSettings();
    }
    else { 
        writeSDSettings();
        readSDSettings();
    };


    // Setup audio 
    waveform1.pulseWidth(0.2);
    waveform2.pulseWidth(0.5);
    waveform3.pulseWidth(0.5);
    waveform4.pulseWidth(0.5);
    waveform5.pulseWidth(0.5);
    waveform6.pulseWidth(0.5);
    waveform7.pulseWidth(0.5);
    waveform8.pulseWidth(0.5);
    /*
    mixer1.gain(0,0.25);
    mixer1.gain(1,0.25);
    mixer1.gain(2,0.25);
    mixer1.gain(3,0.25);
    mixer2.gain(0,0.25);
    mixer2.gain(1,0.25);
    mixer2.gain(2,0.25);
    mixer2.gain(3,0);
    */
    mixer3.gain(0,0.49);
    mixer3.gain(1,0.49);
    mixer3.gain(2,0);
    mixer3.gain(3,0);
    /*
    envelope1.attack(1);
    envelope1.decay(1);
    envelope1.sustain(1.0);
    envelope1.release(1);
    envelope1.noteOn();
    */
    
    if (arpMode) {
      mixer1.gain(0,0.75);
      mixer1.gain(1,0);
      mixer1.gain(2,0);
      mixer1.gain(3,0);
      mixer2.gain(0,0);
      mixer2.gain(1,0);
      mixer2.gain(2,0);
      mixer2.gain(3,0);
      //waveform1.begin(1.0,FREQ[0],wave_type[waveform]);      
    }else {
      mixer1.gain(0,0.25);
      mixer1.gain(1,0.25);
      mixer1.gain(2,0.25);
      mixer1.gain(3,0.25);
      mixer2.gain(0,0.25);
      mixer2.gain(1,0.25);
      mixer2.gain(2,0.25);
      mixer2.gain(3,0);
    }
    
    waveform1.begin(1.0,FREQ[0],wave_type[waveform]);      
    waveform2.begin(1.0,FREQ[1],wave_type[waveform]);
    waveform3.begin(1.0,FREQ[2],wave_type[waveform]);
    waveform4.begin(1.0,FREQ[3],wave_type[waveform]);
    waveform5.begin(1.0,FREQ[4],wave_type[waveform]);
    waveform6.begin(1.0,FREQ[5],wave_type[waveform]);
    waveform7.begin(1.0,FREQ[6],wave_type[waveform]);
    waveform8.begin(1.0,FREQ[7],wave_type[waveform]);
    

    //make the rootMap
    for(int i=0; i<1024; i++) {
      if( i < 12 ) {
        rootMap[i] = 0;
      }else {
        rootMap[i] = (i / 25.95) + 1;
      }
    }
    /*
    for (int i=0; i < 16; i++) {

        Serial.print(notesSD[i][0]);
        Serial.print(notesSD[i][1]);
        Serial.print(notesSD[i][2]);
        Serial.print(notesSD[i][3]);
        Serial.print(notesSD[i][4]);
        Serial.print(notesSD[i][5]);
        Serial.print(notesSD[i][6]);
        Serial.print(notesSD[i][7]);
        

      Serial.println("");
    }
    */

    // Generate the random seed for arp
    randomSeed(analogRead(5));
    SPI.end();
}


void loop(){
    checkInterface();

    int result;


    if (changed) {

        voiceCount = 0;
        //float voiceTotal = 0;
        for(int i = 0; i< SINECOUNT; i++){
            if (notesSD[chordQuant][i] != 999) {
                result = rootQuant + notesSD[chordQuant][i];
                result += chordSpreadList[chordSpread][i];
                FREQ[i] =  numToFreq(result);
                if (i == arpCounter) arpFreq = FREQ[i];
                voiceCount++;
            }
        }
        Serial.print("voice count=");
        Serial.println(voiceCount);
        for (int i = 0; i< SINECOUNT; i++){
            if (notesSD[chordQuant][i] != 999) {
                AMP[i] = 1.0/voiceCount;
                //voiceTotal += 1.0/voiceCount;
            }
            else{
                AMP[i] = 0.0;   
            }
        }

    }


    // CHECK BUTTON STATUS 

    //resetHold = resetHold * resetButton;


    if (shortPress){
        waveform++;
        waveform = waveform % 4;
        ledWrite(waveform);
        changed = true;
        EEPROM.write(1234, waveform);
        shortPress = false;
        AudioNoInterrupts();

	        waveform1.begin(1.0,FREQ[0],wave_type[waveform]);
	        waveform2.begin(1.0,FREQ[1],wave_type[waveform]);
	        waveform3.begin(1.0,FREQ[2],wave_type[waveform]);
	        waveform4.begin(1.0,FREQ[3],wave_type[waveform]);
	        waveform5.begin(1.0,FREQ[4],wave_type[waveform]);
	        waveform6.begin(1.0,FREQ[5],wave_type[waveform]);
	        waveform7.begin(1.0,FREQ[6],wave_type[waveform]);
	        waveform8.begin(1.0,FREQ[7],wave_type[waveform]);
	  
        AudioInterrupts();
    }

    
    if (longPress) {
      arpMode++;
      changed = true;
      if (arpMode > arpNumModes) arpMode = 0;
      longPress = false;
    }

    if (menuB) {

      if(potTurnA) {}
    }
    
    if (changed)  {

        if (arpMode && arpCounter > 0) {
          flashing = false;
          arpPulseArmed = 0;
        }else if (!arpPulseArmed || !arpMode) {
          flashing = true;
          pulseOut = 0;        
          pinMode(RESET_CV, OUTPUT);
          digitalWrite (RESET_LED, HIGH);
          digitalWrite (RESET_CV, HIGH);
        }
        updateSines();

        changed = false;
    }


    if (pulseOut > flashTime && flashing) {
        arpPulseArmed = 1;
        digitalWrite (RESET_LED, LOW);
        digitalWrite (RESET_CV, LOW);
        pinMode(RESET_CV, INPUT);
        flashing = false;  
    } 

}





void updateSines(){
    //    Serial.println("changing");
    //    envelope1.noteOff();
    //    delay(1);

    AudioNoInterrupts();  



    if (arpMode) {
      mixer1.gain(0,0.75);
      mixer1.gain(1,0);
      mixer1.gain(2,0);
      mixer1.gain(3,0);
      mixer2.gain(0,0);
      mixer2.gain(1,0);
      mixer2.gain(2,0);
      mixer2.gain(3,0);

      waveform1.frequency(arpFreq);
      
    }else {
      mixer1.gain(0,AMP[0]);
      mixer1.gain(1,AMP[1]);
      mixer1.gain(2,AMP[2]);
      mixer1.gain(3,AMP[3]);
      mixer2.gain(0,AMP[4]);
      mixer2.gain(1,AMP[5]);
      mixer2.gain(2,AMP[6]);
      mixer2.gain(3,AMP[7]);

      waveform1.frequency(FREQ[0]);
      waveform2.frequency(FREQ[1]);
      waveform3.frequency(FREQ[2]);
      waveform4.frequency(FREQ[3]);
      waveform5.frequency(FREQ[4]);
      waveform6.frequency(FREQ[5]);
      waveform7.frequency(FREQ[6]);
      waveform8.frequency(FREQ[7]);
    }

    AudioInterrupts();
    //    envelope1.noteOn();
    //    delay(1);
    //    printPlaying();

}

float numToFreq(int input) {
    int number = input - 21; // set to midi note numbers = start with 21 at A0 
    number = number - 48; // A0 is 48 steps below A4 = 440hz
    float result; 
    result = 440*(pow (1.059463094359,number));
    return result;   
}

// WRITE A 4 DIGIT BINARY NUMBER TO LED0-LED3 
void ledWrite(int n){
    digitalWrite(LED3, HIGH && (n==0));
    digitalWrite(LED2, HIGH && (n==1));
    digitalWrite(LED1, HIGH && (n==2));
    digitalWrite(LED0, HIGH && (n==3)); 
}

void checkInterface(){

    // Read pots + CVs
    int chordPot = analogRead(CHORD_POT_PIN); 
    int chordCV = analogRead(CHORD_CV_PIN); 
    int rootPot = analogRead(ROOT_POT_PIN); 
    int rootCV = analogRead(ROOT_CV_PIN); 

    if (arpMode) {

      chordRaw = chordPot;
      chordCvRaw = chordCV;
      chordCvRaw = constrain(chordCvRaw, 0, 1023);

      if (chordCvRaw > 1020 && !arpClock) { // new clock pulse received
        arpClock = 1;
        arpClockBounce = 0;
        changed = true;
        
        switch (arpMode) {
        case 1:  // Arp up
          arpCounter++;
          if (arpCounter == voiceCount) arpCounter = 0;          
          break;
          
        case 2:  // Arp down
          arpCounter--;
          if (arpCounter < 0) arpCounter = voiceCount - 1;
          break;

        case 3:  // Ping pong single notes at top and bottom
          arpCounter = arpCounter + arpDirection;
          if (arpCounter == voiceCount) {
            arpDirection = -1;
            arpCounter = voiceCount - 2;
          }else if (arpCounter < 0) {
            arpDirection = 1;
            arpCounter = 1;
          }
          break;

        case 4:  // Ping pong with double trigger and double notes
          arpCounter = arpCounter + arpDirection;
          if (arpCounter == voiceCount) {
            arpDirection = -1;
            arpCounter = voiceCount - 1;
          }else if (arpCounter < 0) {
            arpDirection = 1;
            arpCounter = 0;
          }
          break;
          
        case 5:  // Random values
          arpCounter = random(voiceCount);
          break;

          // also could do 2 up 1 down
          // 3 up 1 down, 3 down 1 up etc
        }          
	
      }else if (chordCvRaw < 100 && arpClock) {  // clock pulse ended 
        if (arpClockBounce < arpClockThresh) arpClockBounce++;
        else arpClock = 0;
      }
      
    }else {

      //chordRaw = chordPot + chordCV;
      chordRaw = chordPot;
      chordCvRaw = chordCV;

      
    }

    //chordRaw = constrain(chordRaw, 0, 1023);
    rootRaw = rootCV;   
    //rootRaw = constrain(rootRaw, 0U, 1023U);

    // Apply hysteresis and filtering to prevent jittery quantization 
    // Thanks to Matthias Puech for this code 

    if ((chordRaw > chordRawOld + 16) || (chordRaw < chordRawOld - 16)){
        chordRawOld = chordRaw;    
    }
    else {
        chordRawOld += (chordRaw - chordRawOld) >>5; 
        chordRaw = chordRawOld;  
    }

    if ((chordCvRaw > chordCvRawOld + 16) || (chordCvRaw < chordCvRawOld - 16)){
        chordCvRawOld = chordCvRaw;    
    }
    else {
        chordCvRawOld += (chordCvRaw - chordCvRawOld) >>5; 
        chordCvRaw = chordCvRawOld;  
    }


    if ((rootPot > rootPotOld + 16) || (rootPot < rootPotOld - 16)){
        rootPotOld = rootPot;    
    }
    else {
        rootPotOld += (rootPot - rootPotOld) >>5; 
        rootPot = rootPotOld;  
    }


    if ((rootRaw > rootRawOld + 16) || (rootRaw < rootRawOld - 16)){
        rootRawOld = rootRaw;    
    }
    else {
        rootRawOld += (rootRaw - rootRawOld) >>5; 
        rootRaw = rootRawOld;  
    }


    
    //chordRawStore = chordRaw;
    //chordQuant = map(chordRaw, 0, 1024, 0, chordCount);
    if (chordCvInput == 0 && !arpMode) {
      if (potLockA) {
        potA = map(constrain(chordQuantStore + chordCvRaw,0, 1023), 0, 1024, 0, chordCount);
      }else {
        potA = map(constrain(chordRaw + chordCvRaw,0, 1023), 0, 1024, 0, chordCount);
      }
    }else {
      if (potLockA) {
        potA = map(chordQuantStore, 0, 1024, 0, chordCount);
      }else {
        potA = map(chordRaw, 0, 1024, 0, chordCount);
      }
    }

    potARaw = map(chordRaw, 0, 1024, 0, chordCount); //need a pot value always

    if (potLockA && abs(chordRaw - chordQuantStore) < 16) { 
      potLockA = false;
      //FLASH LIGHTS
    }
    if (potLockB && abs(rootPot - rootQuantStore) < 16) { 
      potLockB = false;
      //FLASH LIGHTS
    }


    if (potA != chordQuantOld && !buttonState) {
        changed = true;
        chordQuant = potA;
        chordQuantOld = chordQuant;
        chordSpreadPotOld = potARaw;

        if (!potLockA) {  //store the old quant values
          chordQuantStore = chordRaw;
        }
        
    }else if (potARaw != chordSpreadPotOld && buttonState) {
        changed = true;
        potTurnA = true;
        potLockA = true;
        chordSpreadPot = potARaw;
        chordSpreadPotOld = chordSpreadPot;

    }

    // spreading chords
    if (chordCvInput == 1 && !arpMode) {
      chordSpread = chordSpreadPot + map(chordCvRaw, 0, 1024, 0, chordCount);
    }else {
      chordSpread = chordSpreadPot;
    }

    chordSpread = chordSpread % 16; //wrap around instead of holding at last value
    if (chordSpread != chordSpreadOld){
        changed = true; 
        chordSpreadOld = chordSpread;    
    }
    
    //rootQuant = map(rootRaw,0,1024,36,84); // Range = C-2 (36) to C+2 (84)
    //rootQuant = rootMap[rootRaw] + map(rootPot,0,1024,36,84);
    if (potLockB) {
      potB = rootMap[rootRaw] + map(rootQuantStore,0,1024,36,84);
    }else {
      potB = rootMap[rootRaw] + map(rootPot,0,1024,36,84);
    }

    potBRaw = map(rootPot,0,1024,36,84);
    
    if (potB != rootQuantOld && !buttonState) {
        changed = true;
        rootQuant = potB;
        rootQuantOld = rootQuant;
        rootControlOld = potBRaw;
        if (!potLockB) rootQuantStore = rootPot;

    }else if (potBRaw != rootControlOld && buttonState) {
        potTurnB = true;
        potLockB = true;
        changed = true;

        if (rootPot < 100 ) {
          chordCvInput = 0; //control the chord
        }else if (rootPot < 923 ) {
          chordCvInput = 1; //control the spacing
        }else {
          chordCvInput = 2; //control both
        }
        rootControlOld = potBRaw;
    }

    //    resetSwitch.update();
    //    resetButton = resetSwitch.read();



    buttonState = digitalRead(RESET_BUTTON);

    //debounce buttonState here
    
    if (elapsed1 > 10 && buttonState == 0 && lockOut > 999 ){
        shortPress = true;
        if (potTurnA || potTurnB) {
          menuB = true;
          shortPress = false;
          longPress = false;
          potTurnA = false;
          potTurnB = false;
        }
    }

    if (elapsed1 > 1000 && buttonState == 0 && !menuB){
        longPress = true;
        shortPress = false;         //suppress shortPress
        //lockOut = 0;
        //elapsed1 = 0;
    }

    if (!buttonState) {
      menuB = false;
      //      potTurnA = false;
      //      potTurnB = false;
    }
    
    elapsed1 = elapsed1 * buttonState; 

    if (!flashing && RESET_LED) {
        digitalWrite (RESET_LED, LOW);
        digitalWrite (RESET_CV, LOW);
        pinMode(RESET_CV, INPUT);
    }
    /*
    if (!flashing){
        resetCV.update();
        ResetCV = resetCV.rose();
        if (ResetCV) resetFlash = 0; 


        digitalWrite(RESET_LED, (resetFlash<20));
    }
    */
}

//////////////////////////////////////////////
////READ AND WRITE SETTINGS ON THE SD CARD ///
//////////////////////////////////////////////


void readSDSettings(){
    char character;
    int entry = 0;
    int note = 0;
    String settingValue;
    boolean inBracket = false;
    settingsFile = SD.open("CHORDHYP.TXT");
    while (settingsFile.available()) {
        character = settingsFile.read();   
        if (character == '[') {
            inBracket = true; 
        }
        if (character == ','  && inBracket){
            if (entry < 16) {
              notesSD[entry][note] = settingValue.toInt();
            }else {
              chordSpreadList[entry - 16][note] = settingValue.toInt();
            }
            settingValue = "";   
            note++;
        }

        if (character == ']'  && inBracket) {
            if (entry < 16) {
              notesSD[entry][note] = settingValue.toInt();
            } else {
              chordSpreadList[entry - 16][note] = settingValue.toInt();
            }
            settingValue = "";
            entry++;
            note = 0;
            inBracket = false;
        }

        else if (inBracket && character != '[' && character != ',' && character != ']'){
            settingValue += character; 
        }

    }   
    settingsFile.close();
}


// converting string to Float
float toFloat(String settingValue){
    char floatbuf[settingValue.length()];
    settingValue.toCharArray(floatbuf, sizeof(floatbuf));
    float f = atof(floatbuf);
    return f;
}
// Converting String to integer and then to boolean
// 1 = true
// 0 = false
boolean toBoolean(String settingValue) {
    if(settingValue.toInt()==1){
        return true;
    } 
    else {
        return false;
    }
}
// Writes A Configuration file
void writeSDSettings() {
    // Delete the old One
    SD.remove("CHORDHYP.TXT");
    // Create new one
    settingsFile = SD.open("CHORDHYP.TXT", FILE_WRITE);
    //  // writing in the file works just like regular print()/println() function

    settingsFile.println("o   o o   o  o-o  o-O-o   o-o   o-O-o o  o o-O-o o   o  o-o ");
    settingsFile.println("|\\ /| |   | |       |    /        |   |  |   |   |\\  | o    ");
    settingsFile.println("| O | |   |  o-o    |   O         |   O--O   |   | \\ | |  -o ");
    settingsFile.println("|   | |   |     |   |    \\        |   |  |   |   |  \\| o   | ");
    settingsFile.println("o   o  o-o  o--o  o-O-o   o-o     o   o  o o-O-o o   o  o-o ");
    settingsFile.println("");
    settingsFile.println("  o-o o  o  o-o  o--o  o-o        o-o  o--o   o-o    O  o   o ");
    settingsFile.println(" /    |  | o   o |   | |  \\      o   o |   | o      / \\ |\\  | ");
    settingsFile.println("O     O--O |   | O-Oo  |   O     |   | O-Oo  |  -o o---o| \\ | ");
    settingsFile.println(" \\    |  | o   o |  \\  |  /      o   o |  \\  o   | |   ||  \\| ");
    settingsFile.println("  o-o o  o  o-o  o   o o-o        o-o  o   o  o-o  o   oo   o");
    settingsFile.println("");
    settingsFile.println("Edit chord shapes into the spaces below.");
    settingsFile.println("No more than 16 chords and up to 8 notes per chord.");
    settingsFile.println("Anything outside the square brackets is ignored");
    settingsFile.println("Reduce clicks on chord changes by giving all chords");
    settingsFile.println("the same number of notes.");
    settingsFile.println("");

    settingsFile.println("1  [0,4,7,12,0] Major");
    settingsFile.println("2  [0,3,7,12,0] Minor");
    settingsFile.println("3  [0,4,7,11,0] Major 7th");
    settingsFile.println("4  [0,3,7,10,0] Minor 7th");
    settingsFile.println("5  [0,4,7,11,14] Major 9th");
    settingsFile.println("6  [0,3,7,10,14] Minor 9th");
    settingsFile.println("7  [0,5,7,12,0] Suspended 4th");
    settingsFile.println("8  [0,7,12,0,7] Power 5th");
    settingsFile.println("9  [0,5,12,0,5] Power 4th");
    settingsFile.println("10 [0,4,7,8,0] Major 6th");
    settingsFile.println("11 [0,3,7,8,0] Minor 6th");
    settingsFile.println("12 [0,3,6,0,3] Diminished");
    settingsFile.println("13 [0,4,8,0,4] Augmented");
    settingsFile.println("14 [0,0,0,0,0] Root");
    settingsFile.println("15 [-12,-12,0,0,0] Sub Octave");
    settingsFile.println("16 [-12,0,0,12,24] 2 up 1 down octaves");

    settingsFile.println("");
    settingsFile.println("Edit chord voicing in the next space below.");
    settingsFile.println("The firmware now expects 16 chord shapes above and");
    settingsFile.println("16 chord voicings below. Chord voicings should have");
    settingsFile.println("8 notes always. A voicing of 0 does not alter the chord.");
    settingsFile.println("");

    settingsFile.println("1  [0,0,0,0,0,0,0,0] Unaltered voice");
    settingsFile.println("2  [0,12,0,0,0,0,0,0] 2nd note up an Octave");
    settingsFile.println("3  [0,12,0,12,0,0,0,0] 2nd and 4th up an Octave");
    settingsFile.println("4  [0,12,12,12,0,0,0,0] ");
    settingsFile.println("5  [0,12,0,12,12,0,0,0] ");
    settingsFile.println("6  [0,12,12,12,12,0,0,0] ");
    settingsFile.println("7  [12,12,0,12,0,0,0,0] ");
    settingsFile.println("8  [0,7,0,0,0,0,0,0] 2nd note up a 5th");
    settingsFile.println("9  [0,7,7,0,0,0,0,0] 2nd and 3rd up a 5th");
    settingsFile.println("10 [0,1,2,0,0,0,0,0] 2nd up 1 step, 3rd up 2 steps");
    settingsFile.println("11 [-1,1,0,0,0,0,0,0] 1st down a step, 2nd up a step");
    settingsFile.println("12 [-12,12,24,0,0,0,0,0] 1st down an Octave, 2nd up an Octave, 3rd up 2 Octaves");
    settingsFile.println("13 [12,12,0,0,0,0,0,0]");
    settingsFile.println("14 [12,12,12,0,0,0,0,0]");
    settingsFile.println("15 [12,12,12,12,12,12,12,12] All up an Octave");
    settingsFile.println("16 [-12,-12,-12,-12,-12,-12,-12,-12] All down an Octave");
    //


    // close the file:
    settingsFile.close();
    //Serial.println("Writing done.");
}


void reBoot(int delayTime){
    if (delayTime > 0)
        delay (delayTime);
    WRITE_RESTART(0x5FA0004);
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
