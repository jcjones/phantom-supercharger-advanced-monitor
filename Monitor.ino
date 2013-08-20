#include <RunningAverage.h>

#include "Globals.h"

#include <MemoryFree.h>
#include <JCJ_SSD1306.h>
#include <Adafruit_GFX.h>

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

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#if (SSD1306_BUFHEIGHT != 16)
#error("Buf height incorrect, please fix Adafruit_SSD1306.h!");
#endif


Adafruit_SSD1306 display (OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

float sensorTempOneC;
float sensorTempTwoC;
RunningAverage sensorTempOneI(3);
RunningAverage sensorTempTwoI(3);

float sensorVoltOne;
int sensorVoltOneI;


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
  
  Serial.print(F("1:"));  
  Serial.print(sensorTempOneI.getAverage(), 2);  
  Serial.print(" V ");
  Serial.print(sensorTempOneV, 2);
  Serial.print(" C ");
  Serial.print(sensorTempOneC, 2);

  Serial.print(F("    2:"));
  Serial.print(sensorTempTwoI.getAverage(), 2);  
  Serial.print(" V ");
  Serial.print(sensorTempTwoV, 2);
  Serial.print(" C ");
  Serial.println(sensorTempTwoC, 2);
  
}

void readVoltSensors()
{  
  analogRead(VOLT_ONE); // Discard the first read
  sensorVoltOneI = analogRead(VOLT_ONE);

  // fake it
  sensorVoltOneI = (millis()/10)%1023; // fake fake TODO

  sensorVoltOne = sensorVoltOneI * (28.0 / 1024.0);

  addBarGraphDataPoint(sensorVoltOneI);
}

void showMem() {
  Serial.print(F("Free RAM ="));
  Serial.println(freeMemory());
}

void fatalError(char* buf){
  display.clearDisplay();   // clears the screen and buffer

  display.setTextSize(1);
  display.setCursor(0,0);
  display.setTextColor(WHITE); // 'inverted' text
  display.println(buf);
  display.display(); // show splashscreen  
}

void setup()   {       
  Serial.begin(9600);
  analogReference(EXTERNAL);

  showMem();
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  // init done
  display.clearDisplay();   // clears the screen and buffer
  
  // Set the graph callback
  display.setGet8PixelsUnbuffered(getBootPixels);  
  
  // Prepare for text
  display.setTextSize(1);
  display.setCursor(0,0);
  display.setTextColor(WHITE); // 'inverted' text
  display.println(F("      Phantom"));
  display.println(F("   Superchargers"));  
  display.display(); // show splashscreen  
  delay(3000);

  // Configure SD card
  DataLogging_Begin(SD_CS, SD_MOSI, SD_MISO, SD_CLK); 

  // Setup bar graph
  barGraphIndex = 0;
  memset(barGraphData, 0, BARGRAPH_DATA_SZ);

  showMem();
}

uint8_t getBootPixels(int16_t x, int16_t y) {
  if (y % 2 == 0) {
    return 0xFF;
  } 
    
  return 0;
}

// Used to space out disk writes
long lastLogUpdate = 0;

void loop() {
  readTempSensors();
  readVoltSensors();

  display.setGet8PixelsUnbuffered(getGraphPixels);
  
  long time = millis();
  if (time - DISK_WRITE_INTERVAL_MS > lastLogUpdate) {
    lastLogUpdate = time;
    
    // Write to disk
    logData(sensorVoltOne, sensorTempOneC, sensorTempTwoC);
    showMem();    
  }

  // Update the boost info
  updateBoostDuration();
  

  display.clearDisplay();

  display.fillRect(0, 0, display.width(), 16, WHITE);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.setTextColor(BLACK, WHITE); // 'inverted' text

  display.print(F(" ESC "));
  display.print(sensorTempOneC, 0);
  
  display.print(F("C     CPU "));
  display.print(sensorTempTwoC, 0);  
  display.print("C");
  display.println();
  display.print(F("   Last Boost "));  
  display.print(getLastBoostDurationMillis()/1000.0, 2);
  display.println("s");

  drawBars();

  display.setCursor(15, 45);
  display.setTextSize(2);
  display.setTextColor(BLACK, WHITE); // 'inverted' text
  display.print(sensorVoltOne, 2);
  display.println("V");
  display.display();



  // The following line will 'save' the file to the SD card after every
  // line of data - this will use more power and slow down how much data
  // you can read but it's safer! 
  // If you want to speed up the system, remove the call to flush() and it
  // will save the file only every 512 bytes - every time a sector on the 
  // SD card is filled with data.
  /*  dataFile.flush();*/
  
  delay(CYCLE_DELAY);
}







