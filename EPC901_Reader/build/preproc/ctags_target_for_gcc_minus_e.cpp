# 1 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
/*

See the Software notes document for background information

*/
# 4 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
//#include <Arduino.h>
# 6 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 2
# 7 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 2


//#define BUTTON    // ON = use the button for triggering. OFF = use serial stuff


//#define STATUS_PRINTING
//#define CONFIG_PRINTING
//#define DEBUG_PIXEL
//#define DEBUG_ADC



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
const uint_least8_t ADC_CLK = 13; //52;  // ADC clock. AKA SCLK
const uint_least8_t VIDEO_P = A0;
const uint_least8_t VIDEO_N = A1;

// #### Timing Constants #### Note- untrimmed frequency ~~36MHZ --> 1 EPC cycle ~~  27ns
// UNO clock speed: 8MHz --> 1 Arduino cycle = 125 ns; 1 arduino cycle ~~ 4.5 EPC cycles
const uint_least8_t T_STARTUP = 10; // ms
const uint_least8_t T_WAKE_UP = 12; // us
const uint_least16_t T_FLUSH = 1; //889; // us; 30 - 32 EPC cycles ;
const uint_least16_t T_CDS = 2; //1028;  // us; 37 EPC cycles
const uint_least16_t T_SHIFT = 1; //722; // us; 24-26 EPC cycles
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

bool looping = true; // Flag for looping
bool getting_time; // Flag for recieving exposure time via serial
bool success; // Flag for camera success
byte readByte;
uint_least8_t cameraState = 0; // 0 = asleep, 1 = active

void setup()
{
  pinMode(switchPin, 0x0);
  pinMode(PWR_DOWN, 0x1); // HIGH --> power down (sleep) mode
  pinMode(CLR_DATA, 0x1); // active high, rising edge trigger
  pinMode(SHUTTER, 0x1);
  pinMode(CLR_PIX, 0x1); // rising edge trigger
  pinMode(READ, 0x1); // read clock
  pinMode(ADC_CS, 0x1);
  pinMode(ADC_DATA, 0x0);
  pinMode(ADC_CLK, 0x1);

  digitalWrite(PWR_DOWN, 0x1); // chip off

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
    if (('0' <= readByte) && (readByte <= '9')) // Is this a decimal digit?     */
    {
      exposure = 10 * exposure + (readByte & 0x0F); // If so, build time value             */
    } //  end: building time value           */
    else if (readByte == '!')
    {
      getting_time = false;
      Serial.println(exposure);
    } // End time

   }

 }
 else if (Serial.available() > 0)
 {
  readByte = Serial.read(); // read incoming byte

  switch(readByte){
    case 'L': // begin Looping
      Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 160 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 160 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    "Starting looping"
# 160 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    ); &__c[0];}))
# 160 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    )));
      looping = true;
      break;

    case 'Q': // begin Looping
      Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 165 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 165 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    "Quitting looping"
# 165 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    ); &__c[0];}))
# 165 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    )));
      looping = false;
      break;

    case 'W': // Wake
      Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 170 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 170 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    "Waking up EPC901 (ends Serial)"
# 170 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    ); &__c[0];}))
# 170 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    )));
      epcWake();
      break;

    case 'S': // Sleep
      Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 175 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 175 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    "Bedtime for EPC901"
# 175 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    ); &__c[0];}))
# 175 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    )));
      epcSleep();
      break;

   case 'R': // Ready status
      Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 180 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 180 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    "Checking Ready status:"
# 180 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    ); &__c[0];}))
# 180 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    )));
      checkReady((reinterpret_cast<const __FlashStringHelper *>(
# 181 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 181 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                "terminal"
# 181 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                ); &__c[0];}))
# 181 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                )));
      break;

   case 'C': // Capture picture
      Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 185 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 185 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    "Calling capture() and readPicture()"
# 185 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    ); &__c[0];}))
# 185 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    )));
      capture(exposure);
      readPicture();
      printPicture();
      break;

    case 'E': // set Exposure time
      Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 192 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 192 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    "Enter exposure time [microseconds] up to ?-bits. Enter 1000! for 1000 us"
# 192 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    ); &__c[0];}))
# 192 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    )));
      getting_time = true;
      exposure = 0;
      break;


    case '2':
     //Serial.begin(115200);
     //Serial.println(F("Calling epcWake() -> capture(1000) -> readPicture() -> epcSleep()"));
     Serial.end();
     pinMode(DATA_RDY, 0x0);
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
  currentButtonTime = millis(); // waiting to read the button requires a time comparison
  if (currentButtonTime - previousButtonTime > buttonInterval) // compare button refresh time
  {
    previousButtonTime = currentButtonTime; // update previousButtonTime

    if (buttonRead == digitalRead(switchPin)) // verify two reads are the same
    {
      if (buttonRead != buttonState) // Button State has changed!
      {
        buttonState = buttonRead; // Update button state



        if (buttonRead == 0x0) // if button has been RELEASED
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

    Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 277 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                  (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 277 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                  "Taking a picture"
# 277 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                  ); &__c[0];}))
# 277 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                  )));

   checkReady((reinterpret_cast<const __FlashStringHelper *>(
# 279 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
             (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 279 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
             "While asleep in case 1"
# 279 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
             ); &__c[0];}))
# 279 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
             )));
   epcWake(); // includes delays
   capture(exposure);




   readPicture();
   epcSleep();
   //printPicture(picture);
   break;

 case 2: // single press > 1s

    Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 293 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                  (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 293 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                  "Switch case 2 reached: Powering down"
# 293 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                  ); &__c[0];}))
# 293 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                  )));


   checkReady((reinterpret_cast<const __FlashStringHelper *>(
# 296 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
             (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 296 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
             "case 2 beginning"
# 296 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
             ); &__c[0];}))
# 296 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
             )));
   epcSleep();
   checkReady((reinterpret_cast<const __FlashStringHelper *>(
# 298 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
             (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 298 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
             "case 2 end"
# 298 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
             ); &__c[0];}))
# 298 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
             )));
   break;
 }
}



void checkReady(String input)
{
# 317 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
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
# 1 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
void runConfig()
{





  Serial.end();


    pinMode(DATA_RDY, 0x1); // Turns on charge pump @ start if high, signals there is another frame yet to be read out afterwards




  // PINMODE AND SETUP FOR NOT USING ADC
  //pinMode(A1,INPUT);

  // Charge Pump on

    digitalWrite(DATA_RDY, 0x1);


  // defaults
  digitalWrite(CLR_DATA, 0x0);
  digitalWrite(CLR_PIX, 0x0);
  digitalWrite(READ, 0x0);
  digitalWrite(ADC_CS, 0x1); // ADC not selected
  //digitalWrite(SHUTTER, LOW);

  digitalWrite(PWR_DOWN, 0x0); // Turn on EPC901

  delay(10); // Sensor startup time


    digitalWrite(DATA_RDY, 0x0); // config should be done; get DATA_RDY ready for reading
    pinMode(DATA_RDY, 0x0);


  lastFlush = millis();
}

// Configure and wake up chip;  Calls runConfig()
void epcWake()
{
  Serial.end() ; // UNO Serial pins are used by the EPC chip

  runConfig();

  delayMicroseconds(10);
  digitalWrite(PWR_DOWN, 0x0);
  delayMicroseconds(T_WAKE_UP);
  cameraState = 1;
}

void epcSleep()
{
  digitalWrite(PWR_DOWN, 0x1);
  delay(5); // just for good measure.
  cameraState = 0;
}

// flush: clears pixels and frame stores
void flush()
{
  digitalWrite(CLR_PIX, 0x1);
  lastFlush = millis();
  //  delayMicroseconds(1); //shouldn't be necessary. In addition, digitalWrite() takes ~~ 3.4us
  digitalWrite(CLR_PIX, 0x0);
  delayMicroseconds(T_FLUSH - 7); // Flush time - digitalWrite() time
}

// flushBuffer: clears pixels and frame stores
void flushBuffer()
{
  digitalWrite(CLR_DATA, 0x1);
  lastFlush = micros();
  //  delayMicroseconds(1); shouldn't be necessary. In addition, digitalWrite()
  //  takes ~~ 3.4us
  digitalWrite(CLR_DATA, 0x0);
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
  digitalWrite(SHUTTER, 0x1);
  delayMicroseconds(T_FLUSH + exposure - 7); // the -7 is a guess at compensating for digitalWrite()
  digitalWrite(SHUTTER, 0x0);
  //delayMicroseconds(T_SHIFT);
}

// Starts Serial and does a dummy write
void adcStart()
{




  SPI.beginTransaction(SPISettings(10000000, 1, 0x0C)); // Clock polarity = 1, CLK Phase = 1, data output on the falling edge





  readByte = readPixel(); // wake adc
  SPI.endTransaction();





}

// take 1 ADC reading; returns 8-BIT INT
uint_least8_t readPixel() // 
{




  byte temp;
  uint_least16_t val = 0; // the value of the pixel should be (val+1)/4096
  digitalWrite(ADC_CS, 0x0); // Write CS low to start the ADC sample & transmit

  temp = SPI.transfer(0); // read first 8 bits; first 4 are zeroes and not a part of the read

  val = temp; // set val = to high byte

  temp = SPI.transfer(0);

  //val += temp;
  val = (val * 256) + temp; // shift the high byte and add the low byte
  val = val >> 4;
  digitalWrite(ADC_CS, 0x1); // finish this transaction

  return val;
  //return analogRead(A1);
}

// take 1 ADC reading; returns full 12-bit value (with 4 leading 0s)
uint16_t readPixelBig()
{




  byte temp;
  uint16_t val = 0; // the value of the pixel should be (val+1)/4096
  digitalWrite(ADC_CS, 0x0); // Write CS low to start the ADC sample & transmit

  temp = SPI.transfer(0); // read first 8 bits; first 4 are zeroes and not a part of the read

  val = temp; // set val = to high byte

  temp = SPI.transfer(0);

  //val += temp;
  val = (val * 256) + temp; // shift the high byte and add the low byte
  digitalWrite(ADC_CS, 0x1); // finish this transaction

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

  SPI.beginTransaction(SPISettings(10000000, 1, 0x0C));

  // make sure ADC is awoken with a dummy read
  SPI.transfer(0);

  digitalWrite(READ, 0x1);
  delayMicroseconds(1);
  digitalWrite(READ, 0x0);
  delayMicroseconds(2);

  //pre-load sensor pipeline
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(READ, 0x1);
    delayMicroseconds(READ_DELAY);
    digitalWrite(READ, 0x0);
    delayMicroseconds(READ_DELAY);
  }

  // for each pixel: send a READ pulse, read the ADC, print the read
  for (unsigned long i = 0; i < 1024; i++)
  {
    digitalWrite(READ, 0x1);
    __asm__ __volatile__ ("nop\n\t") /* assembly code that does nothing? // should be a smaller delay than delayMicros()*/; __asm__ __volatile__ ("nop\n\t") /* assembly code that does nothing? // should be a smaller delay than delayMicros()*/; // in theory gets a smaller delay than micros, might not be necessary
    digitalWrite(READ, 0x0);
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



  return true;
}
