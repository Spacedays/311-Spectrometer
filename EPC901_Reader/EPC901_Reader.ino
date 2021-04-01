/*
See the Software notes document for background information 
*/

// Variable Declarations

// Breakout Pins
const uint8_t PWR_DOWN = 0;
const uint8_t DATA_RDY = 1;
const uint8_t CLR_DATA = 2;
const uint8_t SHUTTER = 3;
const uint8_t CLR_PIX = 4;
const uint8_t READ = 9;
const uint8_t ADC_CS = 10;
const uint8_t ADC_DATA = 12;  // also MISO
const uint8_t ADC_CLK = 13;   // ADC clock. AKA SCLK
const uint8_t VIDEO_P = A0;   
const uint8_t VIDEO_N = A1;

// #### Timing Constants #### Note- untrimmed frequency ~~36MHZ --> 1 EPC cycle ~~  27ns
//UNO clock speed: 8MHz --> 1 Arduino cycle = 125 ns; 1 arduino cycle ~~ 4.5 EPC cycles
// another note: EPC 

const unsigned int T_STARTUP = 10;  // ms
const unsigned int T_WAKE_UP = 12;  // us
const unsigned long T_FLUSH = 889;       // us; 30 - 32 EPC cycles ; 
const unsigned long T_CDS = 1028; // us; 37 EPC cycles
const unsigned long T_SHIFT = 722;// us; 24-26 EPC cycles 
// T_SHUTTER =  // > 5 EPC clock c-ycles. 

const unsigned int T_PERIOD_FLUSH = 90;  // ms; Should be performed < 100ms ;CLR_PIX pulse should be done frequently
//T_PULSE_CLR_DATA = 83; //us; 3 oscillator cycles; < 1 arduino cycle
const unsigned int READ_DELAY = 3;  // us; arbitrarily assigned. note: digitalWrite() only precise to 4 us & delayMicroseconds() precise above 3us

// #### Timing Variables ####
unsigned long lastFlush;
unsigned long lastBufferFlush;

// Flags
//bool shutterFlag;   // SHUTTER pin
int buttonResult = 0;   // 0 = no change, 1 = pressed < 1s (with debounce), 2 = pressed > 1s 

// Variables
byte temp = 0b00000000;


void setup()
{
  pinMode(PWR_DOWN, OUTPUT);  // HIGH --> power down (sleep) mode
  pinMode(DATA_RDY, INPUT);   // signals there is another frame yet to be read out
  pinMode(CLR_DATA, OUTPUT);  // rising edge trigger
  pinMode(SHUTTER, OUTPUT); 
  pinMode(CLR_PIX, OUTPUT);   // rising edge trigger
  pinMode(READ, OUTPUT);      // read clock
  pinMode(ADC_CS, OUTPUT);
  pinMode(ADC_DATA, INPUT);
  pinMode(ADC_CLK, OUTPUT);

  // defaults
  digitalWrite(PWR_DOWN, LOW);
  digitalWrite(CLR_DATA, LOW);
  digitalWrite(SHUTTER, LOW);
  digitalWrite(CLR_PIX, LOW);
  digitalWrite(READ, LOW);
  digitalWrite(ADC_CS, HIGH);
  digitalWrite(ADC_CLK,HIGH);

  delay(10);    // Sensor startup time
  lastFlush = micros();
}

void loop() 
{
  // flush check

  // end flush check

  // button check logic here  
    switch (buttonResult) 
    {
      case 1:   // single press < 1s
        epcWake();
        delayMicroseconds(T_WAKE_UP);
        capture(1000);
        readPicture();
        epcSleep();
        break;
      case 2:   // single press > 1s
        epcSleep();
        break;
    }
  // end button logic


}

// done, not tested
void flush() 
{
  digitalWrite(CLR_PIX,HIGH);
  lastFlush = micros();
//  delayMicroseconds(1); shouldn't be necessary. In addition, digitalWrite() takes ~~ 3.4us
  digitalWrite(CLR_PIX,LOW);
  delayMicroseconds(T_FLUSH-7);     // Flush time - digitalWrite() time
}

// done, not tested
void flushBuffer() 
{
  digitalWrite(CLR_DATA,HIGH);
  lastFlush = micros();
//  delayMicroseconds(1); shouldn't be necessary. In addition, digitalWrite() takes ~~ 3.4us
  digitalWrite(CLR_DATA,LOW);
}

// done, not tested
bool capture(long exposure)    // exposure time [us]
{
  if(!digitalRead(DATA_RDY)){}
  {
    #ifdef DEBUG 
    Serial.println("Camera is not ready to take an image");
    #endif

    return false;                 // not ready to output an image
  }
  flush();
  digitalWrite(SHUTTER,HIGH);
  delayMicroseconds(T_FLUSH + exposure - 7);  // the -7 is a guess at compensating for digitalWrite()
  digitalWrite(SHUTTER,LOW);
  delayMicroseconds(T_SHIFT);
  
  return true;
}

// WIP
void readPicture()
{
  
  while(micros() - lastFlush < T_CDS){} // do nothing

  for (int i=0;i<3;i++)           // pre-loading sensor pipeline
  {
    digitalWrite(READ,HIGH);
    delayMicroseconds(READ_DELAY);  
    digitalWrite(READ,LOW);
  }
  
  for(int n=0;n<12;n++) 
  {
    digitalWrite(READ,HIGH);
    readByte()
    //delayMicroseconds(READ_DELAY);  
    digitalWrite(READ,LOW);
    
  }
}

// CHECK THE WRITE DIRECTION. Also; WIP
byte readByte()   // Read 1 byte from adc
{
  for(int n=0;n<7;n++)
  {
    bitWrite(temp,n,digitalRead(ADC_DATA)); // note: digitalRead takes ~~5us
  }
  return temp;
}

void epcSleep()
{

}

void epcWake()
{

}

void printResults()
{

}