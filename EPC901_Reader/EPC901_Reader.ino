/*
See the Software notes document for background information
*/
#include "EPC901_Reader.h"
#include <SPI.h>

#define DEBUG
//#define DEBUG_PIXEL
//#define DEBUG_ADC

// Variable Declarations

const unsigned int switchPin = 7;    // button pin
// Breakout Pins
const uint_least8_t PWR_DOWN = 0;
const uint_least8_t DATA_RDY = 1;
const uint_least8_t CLR_DATA = 2;
const uint_least8_t SHUTTER = 3;
const uint_least8_t CLR_PIX = 4;
const uint_least8_t READ = 9;
const uint_least8_t ADC_CS = 10;
const uint_least8_t ADC_DATA = 12; // also MISO
const uint_least8_t ADC_CLK = 13;  // ADC clock. AKA SCLK
const uint_least8_t VIDEO_P = A0;
const uint_least8_t VIDEO_N = A1;

// #### Timing Constants #### Note- untrimmed frequency ~~36MHZ --> 1 EPC cycle ~~  27ns
// UNO clock speed: 8MHz --> 1 Arduino cycle = 125 ns; 1 arduino cycle ~~ 4.5 EPC cycles
const uint_least8_t T_STARTUP = 10; // ms
const uint_least8_t T_WAKE_UP = 12; // us
const uint_least16_t T_FLUSH = 889; // us; 30 - 32 EPC cycles ;
const uint_least16_t T_CDS = 1028;  // us; 37 EPC cycles
const uint_least16_t T_SHIFT = 722; // us; 24-26 EPC cycles
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
uint_least16_t picture[1024];
bool success;       // Flag for camera success
byte test;

void setup() {
  
  Serial.begin(9600);
  Serial.println(F("Initializing"));
  
  pinMode(switchPin, INPUT);
  pinMode(PWR_DOWN, OUTPUT); // HIGH --> power down (sleep) mode
  pinMode(DATA_RDY, INPUT); // Turns on charge pump @ start, signals there is another frame yet to be read out afterwards
  pinMode(CLR_DATA, OUTPUT); // active high, rising edge trigger
  pinMode(SHUTTER, OUTPUT);
  pinMode(CLR_PIX, OUTPUT); // rising edge trigger
  pinMode(READ, OUTPUT);    // read clock
  pinMode(ADC_CS, OUTPUT);
  pinMode(ADC_DATA, INPUT);
  pinMode(ADC_CLK, OUTPUT);

  // Charge Pump on
  //digitalWrite(DATA_RDY, HIGH);

  // defaults
  digitalWrite(PWR_DOWN, HIGH);     // chip off
  digitalWrite(CLR_DATA, LOW);
  digitalWrite(CLR_PIX, LOW);
  digitalWrite(READ, LOW);
  digitalWrite(ADC_CS, HIGH);       // ADC not selected

  //digitalWrite(SHUTTER, LOW);
  
  //digitalWrite(ADC_CLK, HIGH);

  delay(10);  // Sensor startup time
  
  //digitalWrite(DATA_RDY, LOW);    // config should be done
  //pinMode(DATA_RDY, INPUT);       // 

  Serial.println(F("Starting SPI\n"));
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE3));     // Clock polarity = 1, CLK Phase = 1, data output on the falling edge
  Serial.println(F("Calling readPixel()\n"));
  test = readPixel();  // wake adc
  SPI.endTransaction();

  Serial.print(F("Dummy ADC Read with result of "));
  Serial.println(test, BIN);
  
  lastFlush = millis();
  epcSleep();
  buttonState = digitalRead(switchPin);     // Read button
  checkReady("end of Setup()");
}

void loop() {
  // flush check
  if (millis()-lastFlush > T_PERIOD_FLUSH) 
  { 
    flushBuffer();
    /*
    #ifdef DEBUG
      Serial.println(F("Flushed"));
    #endif */
  }
  // end flush check

  // button check logic here
  buttonRead = digitalRead(switchPin);           // read the button
  
  currentButtonTime = millis();             // waiting to read the button requires a time comparison
  if (currentButtonTime - previousButtonTime > buttonInterval) // compare button refresh time
  {
    previousButtonTime = currentButtonTime;                 // update previousButtonTime
    
    if (buttonRead == digitalRead(switchPin))               // verify two reads are the same
    {        
      if (buttonRead != buttonState)                          // Button State has changed!
      {
        buttonState = buttonRead;                               // Update button state
        #ifdef DEBUG_BUTTON
          Serial.println(F("Button Changed"));
        #endif
        if (buttonRead == LOW)                                    // if button has been RELEASED
        {                        
          if(currentButtonTime - buttonPressStart < 1000) {buttonResult = 1;}        // button has been pressed for < 1s
          else{buttonResult = 2;}
        }
        else                                                      // button has been PRESSED
        {
          buttonResult = 0;
          buttonPressStart = millis();
        }
        #ifdef DEBUG_BUTTON
          Serial.println(buttonResult);
          Serial.println();
        #endif
      }
      else {buttonResult = 0;}                                // Button State has not changed; do nothing in the switch statement
      
    }
  } 
  // end button logic
  
  //Serial.println(F("Switching"));
  switch (buttonResult) {
    case 1: // single press < 1s
      #ifdef DEBUG
        Serial.println(F("Switch case 1 reached: Taking a picture"));
      #endif
      checkReady(F("While asleep in case 1"));
      epcWake();
      delayMicroseconds(T_WAKE_UP);
      capture(1000);
      //if(success)                   // If the camera can be read, read the picture
      //{
      Serial.print(F("readPicture() called:\t"));
      Serial.println(readPicture());    
      epcSleep();
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

// done, not tested
// flush: clears pixels and frame stores
void flush() {
  digitalWrite(CLR_PIX, HIGH);
  lastFlush = millis();
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
void capture(long exposure) // exposure time [us]
{
  Serial.println(F("Taking image"));
  flush();  // flush pixels
  digitalWrite(SHUTTER, HIGH);
  delayMicroseconds(T_FLUSH + exposure - 7); // the -7 is a guess at compensating for digitalWrite()
  digitalWrite(SHUTTER, LOW);
  delayMicroseconds(T_SHIFT);
}

// WIP
bool readPicture()//int picture[]) // pass by reference
{
  bool Rdy;
  while (micros() - lastFlush < T_CDS) {} // ensure time to shift to stored pixels has passed
  Rdy = digitalRead(DATA_RDY);
  #ifdef DEBUG
    Serial.print(F("Camera DATA_RDY is reading "));
    Serial.println(String(Rdy));
  #endif

  if (!Rdy) 
  {
    epcSleep();
    Serial.println(F("Data ready is low. returned False.\n"));
    return false; // not ready to output an image
  }
  
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE3));
  
  // make sure ADC is awoken with a dummy read
  SPI.transfer(0);
  
  // pre-load sensor pipeline
  for (int i = 0; i < 2; i++) 
  {
    digitalWrite(READ, HIGH);
    delayMicroseconds(READ_DELAY);
    digitalWrite(READ, LOW);
  }

  // for each pixel: send a READ pulse, read the ADC, print the read 
  for (int i = 0; i < 1024; i++) {
    /*
    Serial.print(readPixel());  // send value
    Serial.write(13);  // carriage return
    Serial.write(10);  // linefeed
    */
    picture[i] = readPixel();
  }
  
  SPI.endTransaction();
  Serial.println(F("Transmission completed!\n"));
  return true;
}

// WIP
unsigned int readPixel() // Read 1 byte from adc
{
  #ifdef DEBUG_PIXEL
    Serial.println(F("Reading pix"));
  #endif
  byte temp;
  unsigned int val = 0;     // the value of the pixel should be (val+1)/4096
  digitalWrite(ADC_CS,LOW);     // Write CS low to start the ADC sample & transmit
  
  temp = SPI.transfer(0);       // read first 8 bits; first 4 are zeroes and not a part of the read
  
  val = temp;                   // set val = to high byte
  
  temp = SPI.transfer(0);
  
  //val += temp; */
  val = (val * 256) + temp;     // shift the high byte and add the low byte
  digitalWrite(ADC_CS, HIGH);   // finish this transaction
  
  
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


void checkReady(String input) 
{
  bool test = digitalRead(DATA_RDY);
  Serial.print(F("Cam has DATA_RDY status "));
  Serial.print(String(test));
  Serial.print(F(" at "));
  Serial.println(input);
  Serial.print(F("\n"));
}

void printPicture(int pic[])
{
  for (int i = 0; i < 1024; i++) {
    Serial.print(pic[i]);  // send value
    Serial.write(13);  // carriage return
    Serial.write(10);  // linefeed
  }
}