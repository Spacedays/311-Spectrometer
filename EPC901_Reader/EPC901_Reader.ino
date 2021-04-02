/*
See the Software notes document for background information
*/
#include "EPC901_Reader.h"
#include <SPI.h>

#define DEBUG

// Variable Declarations

// Breakout Pins
const unsigned int PWR_DOWN = 0;
const unsigned int DATA_RDY = 1;
const unsigned int CLR_DATA = 2;
const unsigned int SHUTTER = 3;
const unsigned int CLR_PIX = 4;
const unsigned int READ = 9;
const unsigned int ADC_CS = 10;
const unsigned int ADC_DATA = 12; // also MISO
const unsigned int ADC_CLK = 13;  // ADC clock. AKA SCLK
const unsigned int VIDEO_P = A0;
const unsigned int VIDEO_N = A1;

// #### Timing Constants #### Note- untrimmed frequency ~~36MHZ --> 1 EPC cycle ~~  27ns
// UNO clock speed: 8MHz --> 1 Arduino cycle = 125 ns; 1 arduino cycle ~~ 4.5 EPC cycles

const unsigned int T_STARTUP = 10; // ms
const unsigned int T_WAKE_UP = 12; // us
const unsigned long T_FLUSH = 889; // us; 30 - 32 EPC cycles ;
const unsigned long T_CDS = 1028;  // us; 37 EPC cycles
const unsigned long T_SHIFT = 722; // us; 24-26 EPC cycles
// T_SHUTTER =  // > 5 EPC clock c-ycles.

const unsigned int T_PERIOD_FLUSH = 90; // ms; Should be performed < 100ms ;CLR_PIX pulse should be done frequently
// T_PULSE_CLR_DATA = 83; //us; 3 oscillator cycles; < 1 arduino cycle
const unsigned int READ_DELAY = 3; // us; arbitrarily assigned. note: digitalWrite() only precise to 4 us & delayMicroseconds() precise above 3us

// #### Timing Variables ####
unsigned long lastFlush;
unsigned long lastBufferFlush;

// Flags
// bool shutterFlag;   // SHUTTER pin
int buttonResult = 0; // 0 = no change, 1 = pressed < 1s (with debounce), 2 = pressed > 1s

// Variables
int picture[1024];

void setup() {
  pinMode(PWR_DOWN, OUTPUT); // HIGH --> power down (sleep) mode
  pinMode(DATA_RDY, INPUT); // signals there is another frame yet to be read out
  pinMode(CLR_DATA, OUTPUT); // rising edge trigger
  pinMode(SHUTTER, OUTPUT);
  pinMode(CLR_PIX, OUTPUT); // rising edge trigger
  pinMode(READ, OUTPUT);    // read clock
  pinMode(ADC_CS, OUTPUT);
  pinMode(ADC_DATA, INPUT);
  pinMode(ADC_CLK, OUTPUT);

  // defaults
  digitalWrite(PWR_DOWN, LOW);
  digitalWrite(CLR_DATA, LOW);
  digitalWrite(SHUTTER, LOW);
  digitalWrite(CLR_PIX, LOW);
  digitalWrite(READ, LOW);
  //digitalWrite(ADC_CS, HIGH);
  //digitalWrite(ADC_CLK, HIGH);

  //readADC(); // dummy ADC read to ensure ADC is not asleep
  delay(10);  // Sensor startup time
  lastFlush = micros();

  #ifdef DEBUG
    Serial.begin(9600);
  #endif
}

void loop() {
  // flush check
  if (millis()-lastFlush > T_PERIOD_FLUSH) { flushBuffer(); }
  // end flush check

  // button check logic here
  switch (buttonResult) {
  case 1: // single press < 1s
    epcWake();
    delayMicroseconds(T_WAKE_UP);
    capture(1000);
    readPicture(picture);
    epcSleep();
    break;
  case 2: // single press > 1s
    epcSleep();
    break;
  }
  // end button logic

}

// done, not tested
// flush: clears pixels and frame stores
void flush() {
  digitalWrite(CLR_PIX, HIGH);
  lastFlush = micros();
  //  delayMicroseconds(1); //shouldn't be necessary. In addition, digitalWrite() takes ~~ 3.4us
  digitalWrite(CLR_PIX, LOW);
  delayMicroseconds(T_FLUSH - 7); // Flush time - digitalWrite() time
}

// done, not tested
// flushBuffer: clears pixels and frame stores
void flushBuffer() {
  digitalWrite(CLR_DATA, HIGH);
  lastFlush = micros();
  //  delayMicroseconds(1); shouldn't be necessary. In addition, digitalWrite()
  //  takes ~~ 3.4us
  digitalWrite(CLR_DATA, LOW);
}

// done, not tested
// capture: 
bool capture(long exposure) // exposure time [us]
{
  if (!digitalRead(DATA_RDY)) 
  {
    #ifdef DEBUG
      Serial.println("Camera is not ready to take an image");
    #endif
    return false; // not ready to output an image
  }
  flush();  // flush pixels
  digitalWrite(SHUTTER, HIGH);
  delayMicroseconds(T_FLUSH + exposure - 7); // the -7 is a guess at compensating for digitalWrite()
  digitalWrite(SHUTTER, LOW);
  delayMicroseconds(T_SHIFT);

  return true;
}

// WIP
void readPicture(int picture[]) // pass by reference
{
  while (micros() - lastFlush < T_CDS) {} // ensure time to shift to stored pixels has passed
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE3));     // Clock polarity = 1, CLK Phase = 1, data output on the falling edge
  
  // make sure ADC is awoken with a dummy read
  SPI.transfer(0);
  
  // pre-load sensor pipeline
  for (int i = 0; i < 3; i++) 
  {
    digitalWrite(READ, HIGH);
    delayMicroseconds(READ_DELAY);
    digitalWrite(READ, LOW);
  }

  // for each pixel: send a READ pulse, read the ADC, store the read 
  for (int i = 0; i < 1024; i++) {
    picture[i] = readPixel();
  }
}

// WIP
unsigned int readPixel() // Read 1 byte from adc
{
  byte temp;
  unsigned int val = 0;     // the value of the pixel should be (val+1)/4096
  digitalWrite(ADC_CS,LOW);     // Write CS low to start the ADC sample & transmit
  
  // 4 leading zeroes
  temp = SPI.transfer(0);       // read first 8 bits; first 4 are zeroes and not a part of the read
  val = temp;                   // set val = to high byte
  temp = SPI.transfer(0);
  //val += temp; */
  val = (val * 256) + temp;    // shift the high byte and add the low byte
  return val;
}

void epcSleep() 
{
  digitalWrite(PWR_DOWN, HIGH);
  delay(1);      // just for good measure.
}

void epcWake() 
{
  digitalWrite(PWR_DOWN, LOW);
  delayMicroseconds(T_WAKE_UP);          
}

void printResults(int picture[]) 
{
  for(int i=0;i<1024;i++)
  {
    Serial.println(picture[i]);
  }
}