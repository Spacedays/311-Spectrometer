/*
Background information is documented in the project report, as well as our team's software notes document
*/
#include <SPI.h>
#include "EPC901.h"

#define CHARGE_PUMP // turns the charge pump ON if defined (we want it on)
//#define BUTTON    // ON = use the button for triggering. OFF = use serial stuff

#define DEBUG
//#define STATUS_PRINTING   // Prints general status messages during operation
//#define CONFIG_PRINTING   // Prints configuration messages during startup (lots of printing)
//#define DEBUG_PIXEL       // Prints when a pixel is read                  (lots of printing)

#define NOP __asm__ __volatile__ ("nop\n\t")    // assembly code that does nothing; should be a smaller delay than delayMicros()

// Variable Declarations

const int switchPin = 7; // button pin
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
const uint_least8_t T_PERIOD_FLUSH = 90; // ms; Should be performed < 100ms ;CLR_PIX pulse should be done frequently as well
// T_PULSE_CLR_DATA = 83; //us; 3 oscillator cycles; < 1 arduino uno cycle, so not necessary
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

// Camera data & control
long exposure = 1000;
const unsigned long picsize = 256;
uint16_t picture[picsize];

// State Control
bool looping = true;      // Flag for looping to print to matlab. Sensor capture & reading occurs every three loops 
bool getting_time;        // Flag for recieving exposure time via serial
bool success;             // Flag for camera success
byte readByte;
uint_least8_t cameraState = 0;  // 0 = asleep, 1 = active. currently only used for debugging

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

  buttonState = digitalRead(switchPin); // Read button
  Serial.begin(115200);
}

unsigned long loops = 0;
void loop()
{
  loops++;

  if (millis() - lastFlush > T_PERIOD_FLUSH) // flush check
  {
    flushBuffer();
  }
  
  readConsole();

  if(looping && loops%3==0) 
    {
      Serial.end();
      // epcSleep();
      // delay(1);
      epcWake();
      
      capture(exposure);
      
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
 if (getting_time)
 {
   if (Serial.available() > 0)
   {
    readByte = Serial.read();
    if (('0' <= readByte) && (readByte <= '9'))             // Is this a decimal digit?     */
    {  
      exposure = 10 * exposure + (readByte & 0x0F);         // If so, build time value             */
    }                                                       //  end: building time value           */
    else if (readByte == '!') 
    {
      getting_time = false;
      Serial.println(exposure);
    }       // End time
    
   }

 }
 else if (Serial.available() > 0)
 {
  readByte = Serial.read(); // read incoming byte
    
  switch(readByte){      
    case 'L':                                     // begin Looping
      Serial.println(F("Starting looping"));
      looping = true;
      break;  

    case 'Q':                                     // begin Looping
      Serial.println(F("Quitting looping"));
      looping = false;
      break;  
      
    case 'W':                                     // Wake
      Serial.println(F("Waking up EPC901 (ends Serial)"));
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

   case 'C':                                      // Capture picture
      Serial.println(F("Calling capture() and readPicture()"));
      capture(exposure);
      readPicture();
      printPicture();
      break;

    case 'E':                                     // set Exposure time
      Serial.println(F("Enter exposure time [microseconds] up to ?-bits. Enter 1000! for 1000 us"));
      getting_time = true;
      exposure = 0;
      break;
   }
 }
}

// Print Picture IF STORED IN ARRAY
void printPicture()
{
  for (unsigned int i = 0; i < picsize; i++)
  {
    Serial.println(picture[i]); // send value
    /*  Sent as a part of the Serial.print() function, but can be explicitly called using Serial.print() instead. Uses more memory.
    //Serial.write(13);  // carriage return
    //Serial.write(10);  // linefeed
    */
  }
  Serial.flush();
}
