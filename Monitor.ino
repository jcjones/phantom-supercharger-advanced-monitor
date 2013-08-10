#include "Globals.h"

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <SD.h>
#include <RTClib.h>

/*
from pins_arduino.h:
 const static uint8_t SS = 10;
 const static uint8_t MOSI = 11;
 const static uint8_t MISO = 12;
 const static uint8_t SCK  = 13;
 */

#if 1

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

#else

/* Solo use */
/*#define OLED_DC 11
 #define OLED_CS 12
 #define OLED_CLK 10
 #define OLED_MOSI 9
 #define OLED_RESET 13
 */
#endif

#define VOLT_ONE A1
#define TEMP_ONE A2
#define TEMP_TWO A3

Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
RTC_DS1307 RTC;

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

float sensorTempOneC;
float sensorTempTwoC;
float sensorVoltOne;
int sensorVoltOneI;

/*File dataFile;*/

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

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  
  Wire.begin();
  RTC.begin();

  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    // uncomment it & upload to set the time, date and start run the RTC!
//    RTC.adjust(DateTime(__DATE__, __TIME__));
  } 
  else {
    DateTime now = RTC.now();
    Serial.print("RTC operating acceptably. Date is ");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();    
  }

  barGraphIndex = 0;
  memset(barGraphData, 0, sizeof(barGraphData));

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);

  // init done
  display.display(); // show splashscreen
  delay(250);
  display.clearDisplay();   // clears the screen and buffer

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("  Phantom");
  display.println("   Super-");
  display.println("  charger");
  display.display();
  delay(500);
  display.clearDisplay();   // clears the screen and buffer

  Serial.println("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
//  pinMode(SS, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(SD_CS, SD_MOSI, SD_MISO, SD_CLK)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1) ;
  }
  Serial.println("Card initialized.");

  // Open up the file we're going to log to!  
//  char buf[12];
//  DateTime now = RTC.now();
//  itoa(now.unixtime(), buf, 10);
//  Serial.print("Opening datalog at ");
//  Serial.println(buf);
  /*dataFile = SD.open(buf, FILE_WRITE);
  if (! dataFile) {
    Serial.println("error opening datalog");
    // Wait forever since we cant write data
    while (1) ;
  }*/
}


void loop()
{
  readTempSensors();
  readVoltSensors();

  /*
  // Write to disk
   dataFile.print(millis());
   dataFile.print(",");
   dataFile.print(sensorVoltOne, 2);
   dataFile.print(",");
   dataFile.print(sensorTempOneC, 2);
   dataFile.print(",");
   dataFile.print(sensorTempTwoC, 2);
   dataFile.println();
   */

  // print to the serial port too
  Serial.print("V: ");  
  Serial.print(sensorVoltOne, 2);
  Serial.print(" SC: ");  
  Serial.print(sensorTempOneC, 2);
  Serial.print(" CPU: ");  
  Serial.print(sensorTempTwoC, 2);
  Serial.println();


  updateBoostDuration();

  display.clearDisplay();

  display.fillRect(0, 0, display.width(), 16, WHITE);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.setTextColor(BLACK, WHITE); // 'inverted' text
  display.print(" ESC ");
  display.print(sensorTempOneC, 0);
  display.print("C     CPU ");
  display.print(sensorTempTwoC, 0);  
  display.print("C");
  display.println();
  display.print("   Last Boost ");  
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





