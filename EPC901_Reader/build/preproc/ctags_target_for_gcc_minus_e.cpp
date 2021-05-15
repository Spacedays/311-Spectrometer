# 1 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
/*

Background information is documented in the project report, as well as our team's software notes document

*/
# 4 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
# 5 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 2
# 6 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 2


//#define BUTTON    // ON = use the button for triggering. OFF = use serial stuff


//#define STATUS_PRINTING   // Prints general status messages during operation
//#define CONFIG_PRINTING   // Prints configuration messages during startup (lots of printing)
//#define DEBUG_PIXEL       // Prints when a pixel is read                  (lots of printing)



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
bool looping = true; // Flag for looping to print to matlab. Sensor capture & reading occurs every three loops 
bool getting_time; // Flag for recieving exposure time via serial
bool success; // Flag for camera success
byte readByte;
uint_least8_t cameraState = 0; // 0 = asleep, 1 = active. currently only used for debugging

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
# 146 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 146 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    "Starting looping"
# 146 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    ); &__c[0];}))
# 146 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    )));
      looping = true;
      break;

    case 'Q': // begin Looping
      Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 151 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 151 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    "Quitting looping"
# 151 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    ); &__c[0];}))
# 151 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    )));
      looping = false;
      break;

    case 'W': // Wake
      Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 156 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 156 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    "Waking up EPC901 (ends Serial)"
# 156 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    ); &__c[0];}))
# 156 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    )));
      epcWake();
      break;

    case 'S': // Sleep
      Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 161 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 161 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    "Bedtime for EPC901"
# 161 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    ); &__c[0];}))
# 161 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    )));
      epcSleep();
      break;

   case 'R': // Ready status
      Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 166 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 166 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    "Checking Ready status:"
# 166 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    ); &__c[0];}))
# 166 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    )));
      checkReady((reinterpret_cast<const __FlashStringHelper *>(
# 167 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 167 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                "terminal"
# 167 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                ); &__c[0];}))
# 167 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                )));
      break;

   case 'C': // Capture picture
      Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 171 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 171 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    "Calling capture() and readPicture()"
# 171 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    ); &__c[0];}))
# 171 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    )));
      capture(exposure);
      readPicture();
      printPicture();
      break;

    case 'E': // set Exposure time
      Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 178 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 178 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    "Enter exposure time [microseconds] up to ?-bits. Enter 1000! for 1000 us"
# 178 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino" 3
                    ); &__c[0];}))
# 178 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
                    )));
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
# 196 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Reader.ino"
  }
  Serial.flush();
}
# 1 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
/*

Core functionality for the EPC901 Sensor using @AStuder's 

*/
# 4 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
// Prep all pins for EPC901 startup. Disables Serial to use the DATA_RDY pin. Should be run before every startup
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

// Calls isDataReady() and prints the status with an inputLocation for debugging. Not safe to call during or around sensor start-up
void checkReady(String inputLocation)
{
# 107 "c:\\Github\\311-Spectrometer\\EPC901_Reader\\EPC901_Core.ino"
}

// Take+Store a picture:
void capture(long exposure) // exposure time [us]
{
  //flush(); // clear pixels                  // Seems to work fine without it
  digitalWrite(SHUTTER, 0x1);
  delayMicroseconds(T_FLUSH + exposure - 7); // the -7 is an approximate compensation for digitalWrite()
  digitalWrite(SHUTTER, 0x0);
  //delayMicroseconds(T_SHIFT);               // 
}

// Starts Serial and does a dummy write. After execution, ADC is ready for reading
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
    __asm__ __volatile__ ("nop\n\t") /* assembly code that does nothing; should be a smaller delay than delayMicros()*/; __asm__ __volatile__ ("nop\n\t") /* assembly code that does nothing; should be a smaller delay than delayMicros()*/; // in theory gets a smaller delay than micros, might not be necessary
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
