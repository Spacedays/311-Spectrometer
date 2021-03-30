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

// Timing Constants. Note- untrimmed frequency ~~36MHZ --> 1 EPC cycle ~~  27ns
//UNO clock speed: 8MHz --> 1 Arduino cycle = 125 ns; 1 arduino cycle ~~ 4.5 EPC cycles
// another note: EPC 

unsigned int T_STARTUP = 10;  // ms
unsigned int T_WAKE_UP = 12;  // us
// T_SHUTTER =  // > 5 EPC clock cycles. 

unsigned int T_PERIOD_FLUSH = 90;  // ms; Should be performed < 100ms ;CLR_PIX pulse should be done frequently

//T_PULSE_CLR_DATA 3 oscillator cycles; < 1 arduino cycle

// Timing Variables
unsigned long lastFlush;

// Flags
//bool shutterFlag;   // SHUTTER pin
int buttonResult = 0;   // 0 = no change, 1 = pressed < 1s (with debounce), 2 = pressed > 1s 


void setup()
{
  pinMode(PWR_DOWN, OUTPUT);
  pinMode(DATA_RDY, INPUT);
  pinMode(CLR_DATA, OUTPUT);
  pinMode(SHUTTER, OUTPUT);
  pinMode(CLR_PIX, OUTPUT);
  pinMode(READ, OUTPUT);
  pinMode(ADC_CS, OUTPUT);
  pinMode(ADC_DATA, OUTPUT);
  pinMode(ADC_CLK, OUTPUT);

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
        capture();
        transmit();
        epcSleep();
        break;
      case 2:   // single press > 1s
        epcSleep();
        break;
    }
  // end button logic


}

void flush()
{
  
}

void capture()
{

}

void transmit()
{
  
}

void epcSleep()
{

}

void printResults()
{

}