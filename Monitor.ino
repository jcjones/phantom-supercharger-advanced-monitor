#include <OneWire.h>

#include <RunningAverage.h>
#include <MemoryFree.h>
#include "Globals.h"
#include "U8glib.h"

/*
from pins_arduino.h:
 const static uint8_t SS = 10;
 const static uint8_t MOSI = 11;
 const static uint8_t MISO = 12;
 const static uint8_t SCK  = 13;
 */

#define SD_CLK SCK
#define SD_MISO MISO
#define SD_MOSI MOSI
#define SD_CS SS
     
/* Use for combination with SD card */
#define OLED_CLK SCK
#define OLED_MOSI MOSI
#define OLED_DC 9
#define OLED_CS 8
#define OLED_RESET 7

OneWire  ds(2);

/*
5V Brown
Gnd S-Blue
Data S-Brown 11
Clk Blue 13
DC Green 9
Rest S-Green 7
CS Orange 8
NC S-Orange
*/


#define VOLT_ONE A3
#define TEMP_ONE A0
#define TEMP_TWO A1
#define AREF_VOLTAGE 3.3

U8GLIB_SSD1306_128X64 u8g(OLED_CLK, OLED_MOSI, OLED_CS, OLED_DC, OLED_RESET); // HW SPI Com: CS = 10, A0 = 9 (Hardware Pins are  SCK = 13 and MOSI = 11)

float sensorTempOneC;
float sensorTempTwoC;
RunningAverage sensorTempOneI(3);
RunningAverage sensorTempTwoI(3);

float sensorVoltOne;
int sensorVoltOneI;

// Used by the Fatal state
char fatalErrorMessage[33];

#define STATE_SPLASH 0
#define STATE_ERROR 8
#define STATE_FATAL 9
#define STATE_NORMAL 1
int displayState;

void changeState(int newState) {
  Serial.print(F("S_OLD: "));
  Serial.print(displayState);  
  Serial.print(F("   S_NEW: "));
  Serial.println(newState);  
  displayState = newState;
}

void readTempSensors()
{
  analogRead(TEMP_ONE); // Discard the first read
  sensorTempOneI.addValue(analogRead(TEMP_ONE));
  
  float sensorTempOneV = sensorTempOneI.getAverage() * AREF_VOLTAGE;
  sensorTempOneV /= 1024.0;
  sensorTempOneC = (sensorTempOneV - 0.5) * 100;

  analogRead(TEMP_TWO); // Discard the first read
  sensorTempTwoI.addValue(analogRead(TEMP_TWO));
  
  float sensorTempTwoV = sensorTempTwoI.getAverage() * AREF_VOLTAGE;
  sensorTempTwoV /= 1024.0;
  sensorTempTwoC = (sensorTempTwoV - 0.5) * 100;  
//  
//  Serial.print(F("1:"));  
//  Serial.print(sensorTempOneI.getAverage(), 2);  
//  Serial.print(" V ");
//  Serial.print(sensorTempOneV, 2);
//  Serial.print(" C ");
//  Serial.print(sensorTempOneC, 2);
//
//  Serial.print(F("    2:"));
//  Serial.print(sensorTempTwoI.getAverage(), 2);  
//  Serial.print(" V ");
//  Serial.print(sensorTempTwoV, 2);
//  Serial.print(" C ");
//  Serial.println(sensorTempTwoC, 2);
  
}

void readVoltSensors()
{  
  analogRead(VOLT_ONE); // Discard the first read
  sensorVoltOneI = analogRead(VOLT_ONE);

  // fake it
  sensorVoltOneI = constrain((millis()/10)%1023, 440, 1023); // fake fake TODO

  sensorVoltOne = sensorVoltOneI * (28.0 / 1024.0);

  addBarGraphDataPoint(sensorVoltOneI);
}

void showMem() {
  Serial.print(F("Free RAM ="));
  Serial.println(freeMemory());
}

void showFatal(const char* buf){
  fatalErrorMessage[0] = '\0';
  strncpy(fatalErrorMessage, buf, sizeof(fatalErrorMessage));
  Serial.print(F("FATAL ERROR:"));
  Serial.println(fatalErrorMessage);
  changeState(STATE_FATAL);
}

void showError(const char* buf){
  fatalErrorMessage[0] = '\0';
  strncpy(fatalErrorMessage, buf, sizeof(fatalErrorMessage));
  Serial.print(F("FATAL ERROR:"));
  Serial.println(fatalErrorMessage);
  changeState(STATE_ERROR);
}

void draw() {
  switch (displayState) {
    case STATE_FATAL:
    case STATE_ERROR:
      u8g.setColorIndex(1);
      u8g.setFont(u8g_font_6x10r);
      u8g.drawFrame(0, 0, DISPLAY_WIDTH, 34);
      u8g.drawStr( 28, 12, F("Fatal Error:"));
      u8g.drawStr( 6, 24, fatalErrorMessage);
      break;
    case STATE_SPLASH:
      u8g.setColorIndex(1);
      u8g.setFont(u8g_font_fur11r);
      u8g.drawStr( 27, 24, F("PHANTOM"));
      u8g.drawStr( 26, 36, F("ELECTRIC"));
      u8g.drawStr( 3, 48, F("SUPERCHARGERS"));      
      break;
    case STATE_NORMAL:
      u8g.setColorIndex(1);
      u8g.setFont(u8g_font_6x10r);
      u8g.setPrintPos(2, 12);
      u8g.print(F("ESC "));
      u8g.print(sensorTempOneC, 1);
      u8g.print(F("C  CPU "));
      u8g.print(sensorTempTwoC, 1);
      u8g.print(F(" C"));
      u8g.setPrintPos(12, 24);      
      u8g.print(F("Last Boost: "));
      u8g.print(getLastBoostDurationMillis()/1000.0, 2);
      u8g.print(F(" s"));
      
      for(int i=0; i<BARGRAPH_DATA_SZ; i++) {
        int h = getBarGraphDataPointInPast(i);
        int x = i*2;
        int yStart = DISPLAY_HEIGHT - h;
        u8g.drawVLine(DISPLAY_WIDTH-x, yStart, h);
        u8g.drawVLine(DISPLAY_WIDTH-x+1, yStart, h);        
      }
      

      u8g.setColorIndex(0);
      u8g.drawRBox(30, 36, 72, 30, 2);

      u8g.setColorIndex(1);
      u8g.drawRFrame(30, 36, 72, 30, 2);      

      u8g.setFont(u8g_font_helvB24n);
      u8g.setPrintPos(34, 63);
      
      u8g.print(sensorVoltOne, 1);
      break;   
  }
}


void setup()   {       
  Serial.begin(9600);
  analogReference(EXTERNAL);
  
  u8g.setHardwareBackup(u8g_backup_avr_spi);
  
  pinMode(SD_CS, OUTPUT);
  pinMode(OLED_CS, OUTPUT);
  
  changeState(STATE_SPLASH);

  // Setup bar graph
  setupBarGraph();

  showMem();
}

// Used to space out disk writes
long lastLogUpdate = 0;
long lastTempUpdate = 0;

void loopNormal() {
  readVoltSensors();
  
  long time = millis();
  
  if (time - TEMP_READ_INTERVAL_MS > lastTempUpdate) {
    lastTempUpdate = time;
    readTempSensors();
  }
  
  if (time - DISK_WRITE_INTERVAL_MS > lastLogUpdate) {
    lastLogUpdate = time;
    
    // Write to disk
    logData(sensorVoltOne, sensorTempOneC, sensorTempTwoC);
    showMem();    
  }

  // Update the boost info
  updateBoostDuration();  
}

void loop() {
  
  // Draw
  u8g.firstPage();  
  do {
    draw();
  } while( u8g.nextPage() );
  
  // State updates
  switch(displayState){
    case STATE_SPLASH:
      changeState(STATE_NORMAL);
      // Initialize the data logger
      DataLogging_Begin(SD_CS, SD_MOSI, SD_MISO, SD_CLK);
      delay(2000);
      break;
    case STATE_FATAL:
      while(1) {
        // Hold here forever
      }
      break;
    case STATE_ERROR:
      changeState(STATE_NORMAL);
      delay(4000);      
      break;
    case STATE_NORMAL:
      loopNormal();    
      break;
   }
  
  delay(CYCLE_DELAY);
}



