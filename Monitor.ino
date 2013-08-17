#include <MemoryFree.h>


#include "Globals.h"

#include <JCJ_SSD1306.h>
#include <Adafruit_GFX.h>

#include <avr/pgmspace.h>

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
#define OLED_CS 4
#define OLED_RESET 7


#define VOLT_ONE A1
#define TEMP_ONE A2
#define TEMP_TWO A3

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#if (SSD1306_BUFHEIGHT != 16)
#error("Buf height incorrect, please fix Adafruit_SSD1306.h!");
#endif

class MyOLED : public Adafruit_SSD1306 {
  public:
    MyOLED(int8_t reset) : Adafruit_SSD1306(reset) {};
    MyOLED(int8_t SID, int8_t SCLK, int8_t DC, int8_t RST, int8_t CS) : Adafruit_SSD1306(SID, SCLK, DC, RST, CS) {};    
    uint8_t get8Pixels(int16_t x, int16_t y);
};

uint8_t MyOLED::get8Pixels(int16_t x, int16_t y) {
  Serial.print(x);
  Serial.print(y);
  if (y < SSD1306_BUFHEIGHT) {
    return Adafruit_SSD1306::get8Pixels(x, y);
  } else {
    return 0xFF;
  }
};

MyOLED display (OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

float sensorTempOneC;
float sensorTempTwoC;
float sensorVoltOne;
int sensorVoltOneI;

void readTempSensors()
{
  analogRead(TEMP_ONE); // Discard the first read
  int sensorTempOneI = analogRead(TEMP_ONE);
  float sensorTempOneV = sensorTempOneI * (5.0 / 1024.0);
  sensorTempOneC = (sensorTempOneV - 0.5) * 100;

  analogRead(TEMP_TWO); // Discard the first read
  int sensorTempTwoI = analogRead(TEMP_TWO);
  float sensorTempTwoV = sensorTempTwoI * (5.0 / 1024.0);
  sensorTempTwoC = (sensorTempTwoV - 0.5) * 100;  
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


void setup()   {      
  Serial.begin(9600);

  showMem();
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  // init done
  display.clearDisplay();   // clears the screen and buffer

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

void loop() {
  readTempSensors();
  readVoltSensors();
  
  // Write to disk
  if ((millis() / 1000l) % 5 == 0 ) {
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







