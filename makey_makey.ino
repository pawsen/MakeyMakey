/*
 ************************************************
 ************** MAKEY MAKEY *********************
 ************************************************
 
 /////////////////////////////////////////////////
 /////////////HOW TO EDIT THE KEYS ///////////////
 /////////////////////////////////////////////////
 - Edit keys in the settings.h file
 - that file should be open in a tab above (in Arduino IDE)
 - more instructions are in that file
 
 //////////////////////////////////////////////////
 ///////// MaKey MaKey FIRMWARE v1.4.1 ////////////
 //////////////////////////////////////////////////
 by: Eric Rosenbaum, Jay Silver, and Jim Lindblom
 MIT Media Lab & Sparkfun
 start date: 2/16/2012 
 current release: 7/5/2012

 Edited to fit Arduino Uno
 */

/////////////////////////
// DEBUG DEFINITIONS ////               
/////////////////////////
//#define DEBUG
//#define DEBUG2 
//#define DEBUG3 
//#define DEBUG_TIMING
//#define DEBUG_MOUSE
//#define DEBUG_TIMING2

////////////////////////
// DEFINED CONSTANTS////
////////////////////////

#define BUFFER_LENGTH    3     // 3 bytes gives us 24 samples
#define NUM_INPUTS       11    //
//#define TARGET_LOOP_TIME 694   // (1/60 seconds) / 24 samples = 694 microseconds per sample 
//#define TARGET_LOOP_TIME 758  // (1/55 seconds) / 24 samples = 758 microseconds per sample 
#define TARGET_LOOP_TIME 744  // (1/56 seconds) / 24 samples = 744 microseconds per sample 

#include "settings.h"

/////////////////////////
// STRUCT ///////////////
/////////////////////////
typedef struct {
  byte pinNumber;
  int keyCode;
  byte measurementBuffer[BUFFER_LENGTH]; 
  boolean oldestMeasurement;
  byte bufferSum;
  boolean pressed;
  boolean prevPressed;
} 
MakeyMakeyInput;

MakeyMakeyInput inputs[NUM_INPUTS];

///////////////////////////////////
// VARIABLES //////////////////////
///////////////////////////////////
int bufferIndex = 0;
byte byteCounter = 0;
byte bitCounter = 0;

int pressThreshold;
int releaseThreshold;
boolean inputChanged;

uint8_t buf[8] = {0}; /* Keyboard report buffer */

// Pin Numbers
int pinNumbers[NUM_INPUTS] = {
  2,3,4,5,6,7,8,9,10,11,12
};

// timing
int loopTime = 0;
int prevTime = 0;
int loopCounter = 0;


///////////////////////////
// FUNCTIONS //////////////
///////////////////////////
void initializeArduino();
void initializeInputs();
void updateMeasurementBuffers();
void updateBufferSums();
void updateBufferIndex();
void updateInputStates();
void addDelay();

//////////////////////
// SETUP /////////////
//////////////////////
void setup() 
{
  initializeArduino();
  initializeInputs();
}

////////////////////
// MAIN LOOP ///////
////////////////////
void loop() 
{
  updateMeasurementBuffers();
  updateBufferSums();
  updateBufferIndex();
  updateInputStates();
  addDelay();
}

//////////////////////////
// INITIALIZE ARDUINO
//////////////////////////
void initializeArduino() {
  /* Also needed for serial.write! Klart, torsk! */
  Serial.begin(9600);

  /* Set up input pins 
   Deactivate the internal pull-ups, since we're using external resistors */
  for (int i=0; i<NUM_INPUTS; i++)
  {
    pinMode(pinNumbers[i], INPUT);
    digitalWrite(pinNumbers[i], LOW);
  }


#ifdef DEBUG
  delay(4000); // allow us time to reprogram in case things are freaking out
#endif


}

///////////////////////////
// INITIALIZE INPUTS
///////////////////////////
void initializeInputs() {

  float thresholdPerc = SWITCH_THRESHOLD_OFFSET_PERC;
  float thresholdCenterBias = SWITCH_THRESHOLD_CENTER_BIAS/50.0;
  float pressThresholdAmount = (BUFFER_LENGTH * 8) * (thresholdPerc / 100.0);
  float thresholdCenter = ( (BUFFER_LENGTH * 8) / 2.0 ) * (thresholdCenterBias);
  pressThreshold = int(thresholdCenter + pressThresholdAmount);
  releaseThreshold = int(thresholdCenter - pressThresholdAmount);

#ifdef DEBUG
  Serial.println("pressTreshold: ");
  Serial.println(pressThreshold);
  Serial.println(releaseThreshold);
  Serial.println("Num inputs:");
#endif

  for (int i=0; i<NUM_INPUTS; i++) {
    inputs[i].pinNumber = pinNumbers[i];
    inputs[i].keyCode = keyCodes[i];

    for (int j=0; j<BUFFER_LENGTH; j++) {
      inputs[i].measurementBuffer[j] = 0;
    }
    inputs[i].oldestMeasurement = 0;
    inputs[i].bufferSum = 0;

    inputs[i].pressed = false;
    inputs[i].prevPressed = false;

#ifdef DEBUG
    Serial.println(i);
#endif

  }
}


//////////////////////////////
// UPDATE MEASUREMENT BUFFERS
//////////////////////////////
void updateMeasurementBuffers() {

  for (int i=0; i<NUM_INPUTS; i++) {

    // store the oldest measurement, which is the one at the current index,
    // before we update it to the new one 
    // we use oldest measurement in updateBufferSums
    byte currentByte = inputs[i].measurementBuffer[byteCounter];
    inputs[i].oldestMeasurement = (currentByte >> bitCounter) & 0x01; 

    // make the new measurement
    boolean newMeasurement = digitalRead(inputs[i].pinNumber);

    // invert so that true means the switch is closed
    newMeasurement = !newMeasurement; 

    // store it    
    if (newMeasurement) {
      currentByte |= (1<<bitCounter);
    } 
    else {
      currentByte &= ~(1<<bitCounter);
    }
    inputs[i].measurementBuffer[byteCounter] = currentByte;
  }
}

///////////////////////////
// UPDATE BUFFER SUMS
///////////////////////////
void updateBufferSums() {

  // the bufferSum is a running tally of the entire measurementBuffer
  // add the new measurement and subtract the old one

  for (int i=0; i<NUM_INPUTS; i++) {
    byte currentByte = inputs[i].measurementBuffer[byteCounter];
    boolean currentMeasurement = (currentByte >> bitCounter) & 0x01; 
    if (currentMeasurement) {
      inputs[i].bufferSum++;
    }
    if (inputs[i].oldestMeasurement) {
      inputs[i].bufferSum--;
    }
  }  
}

///////////////////////////
// UPDATE BUFFER INDEX
///////////////////////////
void updateBufferIndex() {
  bitCounter++;
  if (bitCounter == 8) {
    bitCounter = 0;
    byteCounter++;
    if (byteCounter == BUFFER_LENGTH) {
      byteCounter = 0;
    }
  }
}

///////////////////////////
// UPDATE INPUT STATES
///////////////////////////
void updateInputStates() {
  inputChanged = false;
  boolean multipleKey = false;

  for (int i=0; i<NUM_INPUTS; i++) {
    inputs[i].prevPressed = inputs[i].pressed; // store previous pressed state (only used for mouse buttons)
    /* We can only press one key at the time. Otherwise the computer is spammed. */
    if (inputs[i].pressed && !multipleKey) {
      //buf[2] = 16; /* m  */
      buf[2] = inputs[i].keyCode;
      Serial.write(buf,8);
      if (inputs[i].bufferSum < releaseThreshold) {  
        inputChanged = true;
        inputs[i].pressed = false;
      }
#ifdef DEBUG
      Serial.println(inputs[i].keyCode);
#endif
      multipleKey = true;
    }
    else if (!inputs[i].pressed) {
      if(inputChanged){
        /* Key release */
        buf[2] = 0;
        Serial.write(buf,8);
      }
      if (inputs[i].bufferSum > pressThreshold) {  // input becomes pressed
        inputChanged = true;
        inputs[i].pressed = true; 
      }
    }
  }
#ifdef DEBUG3
  if (inputChanged) {
    Serial.println("change");
  }
#endif
}

/*
///////////////////////////
 // SEND KEY EVENTS (obsolete, used in versions with pro micro bootloader)
 ///////////////////////////
 void sendKeyEvents() {
 if (inputChanged) {
 KeyReport report = {
 0                                                        };
 for (int i=0; i<6; i++) {
 report.keys[i] = 0;
 } 
 int count = 0;
 for (int i=0; i<NUM_INPUTS; i++) {
 if (inputs[i].pressed && (count < 6)) {
 report.keys[count] = inputs[i].keyCode;
 
 #ifdef DEBUG3
 Serial.println(report.keys[count]);
 #endif
 
 count++;        
 }
 }
 if (count > 0) {
 report.modifiers = 0x00;
 report.reserved = 1;
 Keyboard.sendReport(&report);
 } 
 else {
 report.modifiers = 0x00;
 report.reserved = 0;
 Keyboard.sendReport(&report);
 }      
 } 
 else {
 // might need a delay here to compensate for the time it takes to send keyreport
 }
 }
 */

///////////////////////////
// ADD DELAY
///////////////////////////
void addDelay() {

  loopTime = micros() - prevTime;
  if (loopTime < TARGET_LOOP_TIME) {
    int wait = TARGET_LOOP_TIME - loopTime;
    delayMicroseconds(wait);
  }

  prevTime = micros();

#ifdef DEBUG_TIMING
  if (loopCounter == 0) {
    int t = micros()-prevTime;
    Serial.println(t);
  }
  loopCounter++;
  loopCounter %= 999;
#endif

}