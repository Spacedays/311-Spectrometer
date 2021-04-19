// Chip control / main loop execution
void readConsole();
short int readButton();
void buttonSwitch(int);

void checkReady(String);
void printPicture();        // print picture IF STORED IN LOCAL ARRAY

// Core chip functionality
void runConfig();
void epcWake();
void epcSleep();
void flush();
void flushBuffer();
bool isDataReady();
void capture(long);
void adcStart();
uint_least8_t readPixel();
uint16_t readPixelBig();
bool readPicture();//int[]);
