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