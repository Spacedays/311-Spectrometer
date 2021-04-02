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

// OLD bit-banging readPicture
void readPicture(int picture[]) // pass by reference
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
// OLD bit-banging readPicture