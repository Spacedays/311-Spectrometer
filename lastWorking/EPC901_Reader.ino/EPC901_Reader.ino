/*
See the Software notes document for background information
*/
#include <Arduino.h>
#include <SPI.h>
#include "EPC901_Reader.h"
//#include "EPC901_Core.ino"

#define CHARGE_PUMP // turns the charge pump ON if defined (we want it on)
//#define BUTTON    // ON = use the button for triggering. OFF = use serial stuff

#define DEBUG
//#define STATUS_PRINTING
//#define CONFIG_PRINTING
//#define DEBUG_PIXEL
//#define DEBUG_ADC

#define NOP __asm__ __volatile__ ("nop\n\t")    // assembly code that does nothing? // should be a smaller delay than delayMicros()

// Variable Declarations

const int switchPin = 7; // button pin
const int shutTime = 10000;

// Breakout Pins
const uint_least8_t PWR_DOWN = 0;
const uint_least8_t DATA_RDY = 1; // Turns on charge pump @ start if high, signals there is another frame yet to be read out afterwards
const uint_least8_t CLR_DATA = 2;
const uint_least8_t SHUTTER = 3;
const uint_least8_t CLR_PIX = 4;
const uint_least8_t READ = 9;
const uint_least8_t ADC_CS = 10;
const uint_least8_t ADC_DATA = 12; //50; // also MISO
const uint_least8_t ADC_CLK = 13;  //52;  // ADC clock. AKA SCLK
const uint_least8_t VIDEO_P = A0;
const uint_least8_t VIDEO_N = A1;

// #### Timing Constants #### Note- untrimmed frequency ~~36MHZ --> 1 EPC cycle ~~  27ns
// UNO clock speed: 8MHz --> 1 Arduino cycle = 125 ns; 1 arduino cycle ~~ 4.5 EPC cycles
const uint_least8_t T_STARTUP = 10; // ms
const uint_least8_t T_WAKE_UP = 12; // us
const uint_least16_t T_FLUSH = 1;   //889; // us; 30 - 32 EPC cycles ;
const uint_least16_t T_CDS = 2;     //1028;  // us; 37 EPC cycles
const uint_least16_t T_SHIFT = 1;   //722; // us; 24-26 EPC cycles
// T_SHUTTER =  // > 5 EPC clock c-ycles.

const uint_least8_t T_PERIOD_FLUSH = 90; // ms; Should be performed < 100ms ;CLR_PIX pulse should be done frequently
// T_PULSE_CLR_DATA = 83; //us; 3 oscillator cycles; < 1 arduino cycle
const uint_least8_t READ_DELAY = 3; // us; arbitrarily assigned. note: digitalWrite() only precise to 4 us & delayMicroseconds() precise above 3us

// #### Timing Variables ####
unsigned long lastFlush;
unsigned long lastBufferFlush;

// Button Variables
unsigned long buttonPressStart = 0;
unsigned long currentButtonTime = 0;
unsigned long previousButtonTime = 0;
uint_least8_t buttonInterval = 10; // ms
bool buttonRead;
bool buttonState;
uint_least8_t buttonResult = 0; // 0 = no change, 1 = pressed < 1s (with debounce), 2 = pressed > 1s

// Misc
const unsigned long picsize = 256;
uint16_t picture[picsize];
bool success; // Flag for camera success
byte readByte;
uint_least8_t cameraState = 0; // 0 = asleep, 1 = active

void setup()
{
  pinMode(switchPin, INPUT);
  pinMode(PWR_DOWN, OUTPUT); // HIGH --> power down (sleep) mode
  pinMode(CLR_DATA, OUTPUT); // active high, rising edge trigger
  pinMode(SHUTTER, OUTPUT);
  pinMode(CLR_PIX, OUTPUT); // rising edge trigger
  pinMode(READ, OUTPUT);    // read clock
  pinMode(ADC_CS, OUTPUT);
  pinMode(ADC_DATA, INPUT);
  pinMode(ADC_CLK, OUTPUT);

  digitalWrite(PWR_DOWN, HIGH); // chip off

  runConfig();
  adcStart();
  flushBuffer();

  //epcSleep();
  buttonState = digitalRead(switchPin); // Read button

  // DEBUGGING
  Serial.end();
  pinMode(DATA_RDY, INPUT);
  delay(100);
  epcWake();
  
  capture(shutTime);
  
  bool captured = readPicture();

  delay(10);
  Serial.begin(115200);
 
  if (captured)
  {
    //Serial.println(F("Image captured"));
    printPicture();
  }
}

unsigned long loops = 0;
void loop()
{
  //if(!Serial){Serial.begin(115200);}
  loops++;

  if (millis() - lastFlush > T_PERIOD_FLUSH) // flush check
  {
    flushBuffer();
  }

  if(loops%3==0) {
    Serial.end();
    pinMode(DATA_RDY, INPUT);
    delay(10);
    epcWake();
    
    capture(1000);
    
    bool captured = readPicture();
  
    delay(10);
    Serial.begin(115200);
  
    delay(10);
   
    if (captured)
    {
      printPicture();
    }
  }

  delay(100);
} 

// Recieve commands from serial
void readConsole()
{
 if (Serial.available() > 0)
 {
   readByte = Serial.read(); // read incoming byte

  switch(readByte){                
    case 'W':                                     // Wake
      Serial.println(F("Waking up EPC901"));
      epcWake();
      break;
    case 'S':                                     // Sleep
      Serial.println(F("Bedtime for EPC901"));
      epcSleep();
      break;
   case 'R':                                      // Ready status
      Serial.println(F("Checking Ready status:"));
      checkReady(F("terminal"));
      break;
   case 'P':                                      // take Picture
   default:
     //Serial.begin(115200);
     epcWake();
     
     delay(10);
     Serial.begin(115200);
    
     //if (captured)
     //{
     //  //Serial.println(F("Image captured"));
     //  printPicture();
     //}
    break;
   }
 }
}

// Observe the button pin for changes & return the result
short int readButton()
{
  currentButtonTime = millis();                                // waiting to read the button requires a time comparison
  if (currentButtonTime - previousButtonTime > buttonInterval) // compare button refresh time
  {
    previousButtonTime = currentButtonTime; // update previousButtonTime

    if (buttonRead == digitalRead(switchPin)) // verify two reads are the same
    {
      if (buttonRead != buttonState) // Button State has changed!
      {
        buttonState = buttonRead; // Update button state
        #ifdef DEBUG_BUTTON
          Serial.println(F("Button Changed"));
        #endif
        if (buttonRead == LOW) // if button has been RELEASED
        {
          if (currentButtonTime - buttonPressStart < 1000)
          {
            buttonResult = 1;
          } // button has been pressed for < 1s
          else
          {
            buttonResult = 2;
          }
        }
        else // button has been PRESSED
        {
          buttonResult = 0;
          buttonPressStart = millis();
        }
      
      #ifdef DEBUG_BUTTON
        Serial.println(buttonResult);
        Serial.println();
      #endif
      }
      else
      {
        buttonResult = 0;
      } // Button State has not changed; do nothing in the switch statement
    }
  }
  return buttonResult;
}

// Do things from the readButton result
void buttonSwitch(int buttonResult)
{
 switch (buttonResult)
 {
 case 1: // single press < 1s
  #ifdef DEBUG
    Serial.println(F("Taking a picture"));
  #endif
   checkReady(F("While asleep in case 1"));
   epcWake(); // includes delays
   capture(1000);

  #ifdef STATUS_PRINTING
    Serial.print(F("readPicture() called:\t"));
  #endif
   readPicture();
   epcSleep();
   //printPicture(picture);
   break;

 case 2: // single press > 1s
  #ifdef DEBUG
    Serial.println(F("Switch case 2 reached: Powering down"));
  #endif

   checkReady(F("case 2 beginning"));
   epcSleep();
   checkReady(F("case 2 end"));
   break;
 }
}



void checkReady(String input)
{
  #ifdef STATUS_PRINTING
    bool readByte = isDataReady();
    Serial.begin(115200);
    Serial.print(F("Cam has DATA_RDY status "));
    Serial.print(String(readByte));
    Serial.print(F(" at "));
    Serial.println(input);
    Serial.print(F("\n"));
    //Serial.end();
  #endif
}

// Print Picture IF STORED IN ARRAY
void printPicture()
{
  for (unsigned int i = 0; i < picsize; i++)
  {
    Serial.println(picture[i]); // send value
    //Serial.write(13);  // carriage return
    //Serial.write(10);  // linefeed
  }
  Serial.flush();
}
