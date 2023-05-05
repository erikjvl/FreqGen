// Wave generator and frequency meter
// ==================================

// This application is a square wave frequency generator on a ESP32 MCU with a BLE 
// connection to the Android app 'RemoteXY'. It can both generate and measure frequencies. 
//
// The ESP32 is remotely controlled via Bluetooth and the RemoteXY app. In the app the
// output and measured frequencies are displayed. The user interface shows a slider 
// where the output frequency can be set in 201 steps  ranging from 0.1 Hz to 15 MHz.
// All duty cycles are 50%.
//
// When the ESP32 is started the initial 1 Hz frequency is visible on the internal 
// LED (like blink). The Bluetooth connection is named ‘FreqGen’.
//
// By: Erikjvl 2023-05-05
//

#include <Arduino.h>

#include "FreqCountESP.h"
const int inputPin = 14;
const int timerMs = 1000;

#define REMOTEXY_MODE__ESP32CORE_BLE
#include <BLEDevice.h>

#include <RemoteXY.h>

// RemoteXY configurate  
#define REMOTEXY_BLUETOOTH_NAME "FreqGen"
#pragma pack(push, 1)


uint8_t RemoteXY_CONF[] =   // 36 bytes
  { 255,1,0,66,0,29,0,16,28,1,4,32,27,9,11,80,2,26,67,5,
  1,1,61,7,78,28,33,67,5,3,91,58,7,204,28,33 };
  
// Structure defines all the variables and events of the RemoteXY interface 
struct {

    // input variables
  int8_t slider_1; // =-100..100 slider position == 201 positions

    // output variables
  char text_1[33];  // string UTF8 end zero 
  char text_2[33];  // string UTF8 end zero 

    // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0 

} RemoteXY;
#pragma pack(pop)

// END RemoteXY include
// ==============================================================================


// Setting PWM Properties 
const int OutputPin = 2;  
const int PWMChannel = 0;
const int PWMResolution = 2;
const int MAX_DUTY_CYCLE = (int)(pow(2, PWMResolution) - 1);

int prevSliderValue=-1;

double PWMFreqs[] = {0.10,0.11,0.13,0.15,0.17,0.20,0.25,0.30,0.33,0.38,0.40,0.44,0.50,0.55,0.60,0.63,0.67,0.70,0.75,0.80,0.85,0.88,0.90,1.00,1.11,
1.13,1.25,1.33,1.40,1.50,1.60,1.67,1.75,1.80,1.90,2.00,2.25,2.33,2.50,2.67,3.00,3.25,3.33,3.50,3.75,4.00,4.25,4.50,4.75,5.00,
5.25,5.50,5.75,6.00,6.25,6.50,6.75,7.00,7.25,7.50,7.75,8.00,8.25,8.50,8.75,9.00,9.25,9.50,9.75,10.00,10.50,11.00,11.50,12.00,12.50,
13.00,13.50,14.00,14.50,15.00,16.00,17.00,18.00,19.00,20.00,22.50,25.00,27.50,30.00,32.50,35.00,37.50,40.00,42.50,45.00,47.50,50.00,55.00,60.00,65.00,
70.00,75.00,80.00,90.00,100.00,100.00,111.11,125.00,150.00,166.67,200.00,250.00,300.00,333.33,375.00,400.00,444.44,500.00,550.00,600.00,625.00,666.67,700.00,750.00,800.00,
850.00,875.00,900.00,1000.00,1111.11,1125.00,1250.00,1333.33,1400.00,1500.00,1600.00,1666.67,1750.00,1800.00,1900.00,2000.00,2250.00,2333.33,2500.00,2666.67,3000.00,3250.00,3333.33,3500.00,3750.00,
4000.00,4250.00,4500.00,5000.00,5250.00,5500.00,5750.00,6000.00,6250.00,6500.00,6750.00,7000.00,7250.00,7500.00,7750.00,8000.00,8250.00,8500.00,9000.00,9500.00,10000.00,12500.00,15000.00,20000.00,25000.00,
30000.00,35000.00,40000.00,50000.00,60000.00,70000.00,80000.00,90000.00,100000.00,150000.00,200000.00,250000.00,300000.00,400000.00,500000.00,750000.00,
1000000.00,1250000.00,1500000.00,2000000.00,2500000.00,3000000.00,5000000.00,7500000.00,10000000.00,15000000.00 };


void setup() {
  ledcAttachPin(OutputPin, PWMChannel);
   
  FreqCountESP.begin(inputPin, timerMs);  
  RemoteXY_Init ();
  RemoteXY.slider_1 = -77; // init on 1 Hz.
}
  


void loop() 
{ 
  static boolean prev;
  static double makeFreq;
  static unsigned long duration;
  static unsigned long startTime=micros();


  // Measure low frequency 
  // ==============================================================================
  boolean current = digitalRead(inputPin);
  unsigned long eventTime = micros();
  static double lowMeasuredFreq;
  if ((current != prev) &&  ((eventTime - startTime) > 800)){   //debounce time: 800 nanoSec.
    if (current) {
      if (lowMeasuredFreq > 49.0){
        duration = (0.95 * duration) + (0.05 * (eventTime - startTime));
      }  else if (lowMeasuredFreq > 9.0){
        duration = (0.9 * duration) + (0.1 * (eventTime - startTime));
      }  else  duration = eventTime - startTime;
      startTime = eventTime + 1;
      lowMeasuredFreq = 1000000.0 / duration;
    }
    prev = current;
  }
  
  // Make low frequency out square wave, with C code up to 250Hz. 
  // Above 250 Hz. wave is made by Ledc in the GUI code block.
  // ==============================================================================
  if (makeFreq < 250.0){
    digitalWrite(OutputPin, ((long)((makeFreq * micros())/500000.0)) %2);
  }


  // Handle BLU Gui on Android app RemoteXY  
  // ==============================================================================
  if (prevSliderValue != RemoteXY.slider_1){
    makeFreq = PWMFreqs[RemoteXY.slider_1+100];
    
    if (makeFreq >= 250.0){
      ledcAttachPin(OutputPin, PWMChannel);
      ledcSetup(PWMChannel, makeFreq, PWMResolution);
      ledcWrite(PWMChannel, (MAX_DUTY_CYCLE+1)/2);

    } else {
        ledcDetachPin(OutputPin);
        pinMode(OutputPin, OUTPUT);
    }


    if(makeFreq > 999999.9) {
       sprintf(RemoteXY.text_1, "Pin %d Output: %2.4f MHz.", OutputPin,  (double) makeFreq/1000000.0);
    } else if (makeFreq > 999.99) {
      sprintf(RemoteXY.text_1, "Pin %d Output: %3.3f KHz.", OutputPin,  (double) makeFreq/1000.0); 
    } else {
      sprintf(RemoteXY.text_1, "Pin %d Output: %3.3f Hz.",  OutputPin, (double) makeFreq); 
    }

    prevSliderValue = RemoteXY.slider_1;
  }

  if (FreqCountESP.available()){
    double measuredFreq = FreqCountESP.read();
    if (measuredFreq > 999999) {
      sprintf(RemoteXY.text_2, "Pin %d Input: %2.4f MHz.", inputPin, (double) measuredFreq/1000000.0);
    } else if (measuredFreq > 999) {
      sprintf(RemoteXY.text_2, "Pin %d Input: %3.3f KHz.", inputPin, (double) measuredFreq/1000.0);
    } else if (measuredFreq > 169) {
      sprintf(RemoteXY.text_2, "Pin %d Input: %3.0f Hz.",  inputPin, (double) measuredFreq);
    } else if (0.099 < lowMeasuredFreq) {
        sprintf(RemoteXY.text_2, "Pin %d Input: %2.2f Hz.", inputPin, lowMeasuredFreq);
    } else {
        sprintf(RemoteXY.text_2, "Pin %d Input freq too low.", inputPin, lowMeasuredFreq);
    }
  }
  RemoteXY_Handler(); 
}