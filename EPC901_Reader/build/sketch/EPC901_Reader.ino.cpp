#include <Arduino.h>
#line 1 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
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

#line 61 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
void setup();
#line 83 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
void loop();
#line 120 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
void readConsole();
#line 187 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
void printPicture();
#line 5 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
void runConfig();
#line 48 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
void epcWake();
#line 60 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
void epcSleep();
#line 68 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
void flush();
#line 78 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
void flushBuffer();
#line 88 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
bool isDataReady();
#line 94 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
void checkReady(String inputLocation);
#line 110 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
void capture(long exposure);
#line 120 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
void adcStart();
#line 142 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
uint_least8_t readPixel();
#line 168 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
uint16_t readPixelBig();
#line 194 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
bool readPicture();
#line 61 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
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

#line 1 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
/*
Core functionality for the EPC901 Sensor using @AStuder's 
*/
// Prep all pins for EPC901 startup. Disables Serial to use the DATA_RDY pin. Should be run before every startup
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

// Calls isDataReady() and prints the status with an inputLocation for debugging. Not safe to call during or around sensor start-up
void checkReady(String inputLocation)
{
  #ifdef STATUS_PRINTING
    bool readByte = isDataReady();
    bool isSerialOff = if(!Serial);  
    Serial.begin(115200);
    Serial.print(F("Cam has DATA_RDY status "));
    Serial.print(String(readByte));
    Serial.print(F(" at "));
    Serial.println(inputLocation);
    Serial.print(F("\n"));
    if (isSerialOff) {Serial.end();}
  #endif
}

// Take+Store a picture:
void capture(long exposure) // exposure time [us]
{
  //flush(); // clear pixels                  // Seems to work fine without it
  digitalWrite(SHUTTER, HIGH);
  delayMicroseconds(T_FLUSH + exposure - 7);  // the -7 is an approximate compensation for digitalWrite()
  digitalWrite(SHUTTER, LOW);
  //delayMicroseconds(T_SHIFT);               // 
}

// Starts Serial and does a dummy write. After execution, ADC is ready for reading
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
