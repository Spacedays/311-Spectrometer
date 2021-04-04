int x;
//uint_least8_t pic[1024];

void setup() {
  
  Serial.begin(9600);  
}

void loop() 
{ 
  Serial.println("Calculating variables");
  for(float i = 0;i<1024;i++)
  {
    // print values in an offset, downward facing parabola with the max of 4096 at i = 512, or the 513th "pixel" of the sensor
    //pic[int(i)]  = int(-0.015625*sq(i-512)+4096);
    x = int(-0.015625*sq(i-512)+4096);
    Serial.print(x);  // send value
    Serial.write(13);  // carriage return
    Serial.write(10);  // linefeed
  }
  //Serial.println(F("\nPrinting Values\n"));

  /*
  for(int i = 0;i<1024;i++)
  {
    Serial.print(pic[i]);  // send value
    Serial.write(13);  // carriage return
    Serial.write(10);  // linefeed
  }
  //Serial.println(F("\nFinished Printing Values\n"));
  
  memset(pic,0,sizeof(pic));
  */
  delay(10000);
}
