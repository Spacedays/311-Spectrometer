#include <Arduino.h>
#line 1 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
/*
See the Software notes document for background information
*/
//#include <Arduino.h>
#include <SPI.h>
#include "EPC901.h"

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

long exposure = 1000;
const unsigned long picsize = 256;
uint16_t picture[picsize];

bool looping = true;       // Flag for looping
bool getting_time;  // Flag for recieving exposure time via serial
bool success; // Flag for camera success
byte readByte;
uint_least8_t cameraState = 0; // 0 = asleep, 1 = active

#line 73 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
void setup();
#line 96 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
void loop();
#line 134 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
void readConsole();
#line 224 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
short int readButton();
#line 271 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
void buttonSwitch(int buttonResult);
#line 305 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
void checkReady(String input);
#line 320 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
void printPicture();
#line 1 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
void runConfig();
#line 44 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
void epcWake();
#line 56 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
void epcSleep();
#line 64 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
void flush();
#line 74 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
void flushBuffer();
#line 84 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
bool isDataReady();
#line 90 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
void capture(long exposure);
#line 100 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
void adcStart();
#line 122 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
uint_least8_t readPixel();
#line 148 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
uint16_t readPixelBig();
#line 174 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
bool readPicture();
#line 73 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
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
  //if(!Serial){Serial.begin(115200);}
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


    case '2':
     //Serial.begin(115200);
     //Serial.println(F("Calling epcWake() -> capture(1000) -> readPicture() -> epcSleep()"));
     Serial.end();
     pinMode(DATA_RDY, INPUT);
     delay(100);
     epcWake();
     
     capture(exposure);
     
     bool captured = readPicture();
 
     delay(10);
     Serial.begin(115200);
    
     if (captured)
     {
       //Serial.println(F("Image captured"));
       printPicture();
     }
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
   capture(exposure);

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

#line 1 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
void runConfig()
{
  #ifdef CONFIG_PRINTING
    Serial.end();
    Serial.begin(115200);
    Serial.println(F("Initializing Sensor"));
  #endif
  Serial.end();

  #ifdef CHARGE_PUMP
    pinMode(DATA_RDY, OUTPUT); // Turns on charge pump @ start if high, signals there is another frame yet to be read out afterwards
  #else
    pinMode(DATA_RDY, INPUT);
  #endif

  // PINMODE AND SETUP FOR NOT USING ADC
  //pinMode(A1,INPUT);

  // Charge Pump on
  #ifdef CHARGE_PUMP
    digitalWrite(DATA_RDY, HIGH);
  #endif

  // defaults
  digitalWrite(CLR_DATA, LOW);
  digitalWrite(CLR_PIX, LOW);
  digitalWrite(READ, LOW);
  digitalWrite(ADC_CS, HIGH);       // ADC not selected
  //digitalWrite(SHUTTER, LOW);

  digitalWrite(PWR_DOWN, LOW);      // Turn on EPC901

  delay(10); // Sensor startup time

  #ifdef CHARGE_PUMP
    digitalWrite(DATA_RDY, LOW); // config should be done; get DATA_RDY ready for reading
    pinMode(DATA_RDY, INPUT);
  #endif

  lastFlush = millis();
}

// Configure and wake up chip;  Calls runConfig()
void epcWake()
{
  Serial.end() ;                  // UNO Serial pins are used by the EPC chip

  runConfig();

  delayMicroseconds(10);
  digitalWrite(PWR_DOWN, LOW);
  delayMicroseconds(T_WAKE_UP);
  cameraState = 1;
}

void epcSleep()
{
  digitalWrite(PWR_DOWN, HIGH);
  delay(5); // just for good measure.
  cameraState = 0;
}

// flush: clears pixels and frame stores
void flush()
{
  digitalWrite(CLR_PIX, HIGH);
  lastFlush = millis();
  //  delayMicroseconds(1); //shouldn't be necessary. In addition, digitalWrite() takes ~~ 3.4us
  digitalWrite(CLR_PIX, LOW);
  delayMicroseconds(T_FLUSH - 7); // Flush time - digitalWrite() time
}

// flushBuffer: clears pixels and frame stores
void flushBuffer()
{
  digitalWrite(CLR_DATA, HIGH);
  lastFlush = micros();
  //  delayMicroseconds(1); shouldn't be necessary. In addition, digitalWrite()
  //  takes ~~ 3.4us
  digitalWrite(CLR_DATA, LOW);
}

// Reads DATA_RDY pin
bool isDataReady()
{
  return digitalRead(DATA_RDY); // HIGH ==> ready
}

// Take+Store a picture:
void capture(long exposure) // exposure time [us]
{
  //flush(); // clear pixels                  // Seems to work fine without it
  digitalWrite(SHUTTER, HIGH);
  delayMicroseconds(T_FLUSH + exposure - 7);  // the -7 is a guess at compensating for digitalWrite()
  digitalWrite(SHUTTER, LOW);
  //delayMicroseconds(T_SHIFT);
}

// Starts Serial and does a dummy write
void adcStart()
{
  #ifdef CONFIG_PRINTING
    Serial.println(F("Starting SPI for Dummy Read\n"));
  #endif

  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE3)); // Clock polarity = 1, CLK Phase = 1, data output on the falling edge

  #ifdef CONFIG_PRINTING
    Serial.println(F("About to call readPixel()"));
  #endif

  readByte = readPixel(); // wake adc
  SPI.endTransaction();

  #ifdef STATUS_PRINTING
    Serial.print(F("Startup Dummy ADC Read with result of "));
    Serial.println(readByte, BIN);
  #endif
}

// take 1 ADC reading; returns 8-BIT INT
uint_least8_t readPixel() // 
{
  #ifdef DEBUG_PIXEL
    Serial.println(F("Reading pix"));
  #endif

  byte temp;
  uint_least16_t val = 0;    // the value of the pixel should be (val+1)/4096
  digitalWrite(ADC_CS, LOW); // Write CS low to start the ADC sample & transmit

  temp = SPI.transfer(0); // read first 8 bits; first 4 are zeroes and not a part of the read

  val = temp; // set val = to high byte

  temp = SPI.transfer(0);

  //val += temp;
  val = (val * 256) + temp; // shift the high byte and add the low byte
  val = val >> 4;
  digitalWrite(ADC_CS, HIGH); // finish this transaction

  return val;
  //return analogRead(A1);
}

// take 1 ADC reading; returns full 12-bit value (with 4 leading 0s)
uint16_t readPixelBig() 
{
  #ifdef DEBUG_PIXEL
    Serial.println(F("Reading pix"));
  #endif

  byte temp;
  uint16_t val = 0;          // the value of the pixel should be (val+1)/4096
  digitalWrite(ADC_CS, LOW); // Write CS low to start the ADC sample & transmit

  temp = SPI.transfer(0); // read first 8 bits; first 4 are zeroes and not a part of the read

  val = temp; // set val = to high byte

  temp = SPI.transfer(0);

  //val += temp;
  val = (val * 256) + temp;   // shift the high byte and add the low byte
  digitalWrite(ADC_CS, HIGH); // finish this transaction

  return val;
  //return analogRead(A1);
}

// If DATA_RDY is HIGH, reads from sensor; STATUS_PRINTING results
// calls readPixel() if DATA_RDY, calls epcSleep() if not
bool readPicture() //int picture[]) // pass by reference
{
  while (micros() - lastFlush < T_CDS) {} // ensure time to shift to stored pixels has passed

  bool Rdy = isDataReady();
  if (!Rdy)
  {
    epcSleep();
    //Serial.println(F("Data ready is low. returned False.\n"));
    return false; // not ready to output an image
  }

  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE3));

  // make sure ADC is awoken with a dummy read
  SPI.transfer(0);

  digitalWrite(READ, HIGH);
  delayMicroseconds(1);
  digitalWrite(READ, LOW);
  delayMicroseconds(2);

  //pre-load sensor pipeline
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(READ, HIGH);
    delayMicroseconds(READ_DELAY);
    digitalWrite(READ, LOW);
    delayMicroseconds(READ_DELAY);
  }

  // for each pixel: send a READ pulse, read the ADC, print the read
  for (unsigned long i = 0; i < 1024; i++)
  {
    digitalWrite(READ, HIGH);
    NOP; NOP;                 // in theory gets a smaller delay than micros, might not be necessary
    digitalWrite(READ, LOW);
    //delayMicroseconds(3);
    if (i >= 384 && i < 640)
    {
    picture[i-384] = readPixelBig();
    }
    else
    {
      readPixelBig();
    }
    //Serial.println(readPixelBig()); // send value
    //Serial.write(13);  // carriage return
    //Serial.write(10);  // linefeed

    //picture[i] = readPixel();
  }

  SPI.endTransaction();
  #ifdef STATUS_PRINTING
    Serial.println(F("Transmission completed!\n"));
  #endif
  return true;
}