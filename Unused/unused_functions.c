// BUTTON STUFF

// Button Variables
unsigned long buttonPressStart = 0;
unsigned long currentButtonTime = 0;
unsigned long previousButtonTime = 0;
uint_least8_t buttonInterval = 10; // ms
bool buttonRead;
bool buttonState;
uint_least8_t buttonResult = 0; // 0 = no change, 1 = pressed < 1s (with debounce), 2 = pressed > 1s

buttonState = digitalRead(switchPin); // Read button

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


// END OF BUTTON STUFF
unsigned int readADC() // Read 1 byte from adc
{
  int val = 0x000000000000;     // shown in binary for clarity; the value of the pixel is (val+1)/4096
  digitalWrite(ADC_CS,LOW);
  

  // 4 leading zeroes
  for (int i = 0; i < 3; i++) 
  {
    digitalWrite(ADC_CLK,LOW);
    delayMicroseconds(READ_DELAY);
    digitalWrite(ADC_CLK, HIGH);
  }
  
  for (int n = 12; n > 0; n--)  // 12 bit message, MSB First
  {
    digitalWrite(ADC_CLK,LOW);
    bitWrite(val, n, digitalRead(ADC_DATA)); // note: digitalRead takes ~~5us. accessing the port to read would be faster
    digitalWrite(ADC_CLK, HIGH);
  }
  return val;
}

// OLD readPicture
void readPictureBitBang(int picture[]) // pass by reference
{
  while (micros() - lastFlush < T_CDS) {} // ensure time to shift to stored pixels has passed

  // pre-load sensor pipeline
  for (int i = 0; i < 3; i++) 
  {
    digitalWrite(READ, HIGH);
    delayMicroseconds(READ_DELAY);
    digitalWrite(READ, LOW);
  }

  // for each pixel: send a READ pulse, read the ADC, store the read 
  for (int i = 0; i < 1024; i++) {
    digitalWrite(READ, HIGH);
    delayMicroseconds(READ_DELAY);
    digitalWrite(READ, LOW);
    picture[i] = readADC();
  }
}

// Button reading from main loop
/*
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
  */
