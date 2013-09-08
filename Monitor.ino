#include <OneWire.h>
#include <MemoryFree.h>
#include "Globals.h"
#include "U8glib.h"


/* Analog */
#define VOLT_ONE A3
#define AREF_VOLTAGE 5.0

/* Digital */
#define BUTTON 2
#define ONEWIRE 3


/* Hardware SPI */
#define SD_CLK SCK
#define SD_MISO MISO
#define SD_MOSI MOSI
#define SD_CS SS
     
/* Software SPI */
#define OLED_CLK SCK
#define OLED_MOSI MOSI
#define OLED_DC 9
#define OLED_CS 8
#define OLED_RESET 7

OneWire  oneWire(ONEWIRE);
/* OneWire scratchpad */
byte scratchPad[9];

/* Addresses for the 1-wire temperature probes */
byte controllerThermometer[] = { 0x28, 0xF1, 0x14, 0xB3, 0x04, 0x00, 0x00, 0xBF };
byte motorThermometer[] = { 0x28, 0x0D, 0xDE, 0xB2, 0x04, 0x00, 0x00, 0xA0 };

/* Used to space out periodic tasks */
long lastLogUpdate = 0;
long lastTempUpdate = 0;

/*
from pins_arduino.h:
 const static uint8_t SS = 10;
 const static uint8_t MOSI = 11;
 const static uint8_t MISO = 12;
 const static uint8_t SCK  = 13;
 */
 
/*
Conversion Table
------------------------------------
SPI | Cat5 wire colors | Digital Pin
------------------------------------
5V  | Brown            | -
Gnd | S-Blue           | -
Data| Orange           | 11
Clk | Blue             | 13
DC  | Green            | 9
Rest| S-Green          | 7
CS  | S-Brown          | 8
NC  | S-Orange         | NC (button)
*/

/* Screen */
U8GLIB_SSD1306_128X64 u8g(OLED_CLK, OLED_MOSI, OLED_CS, OLED_DC, OLED_RESET); // HW SPI Com: CS = 10, A0 = 9 (Hardware Pins are  SCK = 13 and MOSI = 11)

float controllerTempC;
float motorTempC;
float sensorVoltOne;
int sensorVoltOneI;


// Used by the Fatal state
char onscreenNoticeMessage[33];

#define STATE_SPLASH 0
#define STATE_NOTICE 8
#define STATE_FATAL 9
#define STATE_NORMAL 1
int displayState;

/* State change function */
void changeState(int newState) {
  Serial.print(F("S_OLD: "));
  Serial.print(displayState);  
  Serial.print(F("   S_NEW: "));
  Serial.println(newState);  
  displayState = newState;
}

/* Set the temperature probes to high resolution */
void configureTempResolution() {
  scratchPad[CONFIGURATION] = TEMP_12_BIT;

  oneWire.reset();
  oneWire.skip(); 
  oneWire.write(WRITESCRATCH);
  
  oneWire.write(scratchPad[HIGH_ALARM_TEMP]); // high alarm temp
  oneWire.write(scratchPad[LOW_ALARM_TEMP]); // low alarm temp
  oneWire.write(scratchPad[CONFIGURATION]); // configuration
  
  oneWire.reset();
  oneWire.skip();
  oneWire.write(COPYSCRATCH, NOT_PARASITE);
} 

/* Tell the probes to sample the temperature and store it in their EEPROM */
void requestTemps() {
  
  oneWire.reset();
  oneWire.skip();
  oneWire.write(STARTCONVO, NOT_PARASITE);
}


/* Read the EEPROM of one of the probes, identified by the address */
float readTemp(byte *addr) {
  byte crc;
  byte i;
  
  oneWire.reset();
  oneWire.select(addr);
  oneWire.write(READSCRATCH);
  // read the response

  // byte 0: temperature LSB
  scratchPad[TEMP_LSB] = oneWire.read();

  // byte 1: temperature MSB
  scratchPad[TEMP_MSB] = oneWire.read();

  // byte 2: high alarm temp
  scratchPad[HIGH_ALARM_TEMP] = oneWire.read();

  // byte 3: low alarm temp
  scratchPad[LOW_ALARM_TEMP] = oneWire.read();

  // byte 4:
  // DS18S20: store for crc
  // DS18B20 & DS1822: configuration register
  scratchPad[CONFIGURATION] = oneWire.read();

  // byte 5:
  // internal use & crc
  scratchPad[INTERNAL_BYTE] = oneWire.read();

  // byte 6:
  // DS18S20: COUNT_REMAIN
  // DS18B20 & DS1822: store for crc
  scratchPad[COUNT_REMAIN] = oneWire.read();

  // byte 7:
  // DS18S20: COUNT_PER_C
  // DS18B20 & DS1822: store for crc
  scratchPad[COUNT_PER_C] = oneWire.read();

  // byte 8:
  // SCTRACHPAD_CRC
  scratchPad[SCRATCHPAD_CRC] = oneWire.read();

  oneWire.reset();
  
  crc = OneWire::crc8(scratchPad, 8);
  if (crc != scratchPad[8]) {
    return 0.0;
  }
  
  int16_t raw = (scratchPad[TEMP_MSB] << 8) | scratchPad[TEMP_LSB];

  byte cfg = (scratchPad[4] & 0x60);
  // at lower res, the low bits are undefined, so let's zero them
  if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
  else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
  else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
  
  return (float) raw / 16.0;
}

/* Read all temperature probes and fill in the global variables */
void readTempSensors()
{
  controllerTempC = readTemp(controllerThermometer);
  motorTempC = readTemp(motorThermometer);
}

/* Read all the voltage sensors and fill in the global variables */
void readVoltSensors()
{  
  analogRead(VOLT_ONE); // Discard the first read
  sensorVoltOneI = analogRead(VOLT_ONE);

  // fake it
//  sensorVoltOneI = constrain((millis()/10)%1023, 440, 1023); // fake fake TODO
  sensorVoltOneI = (int)(292.0*sin(millis()/5000.0)+732.0);

  sensorVoltOne = sensorVoltOneI * (30.0 / 1024.0);

  addBarGraphDataPoint(sensorVoltOneI);
}

/* Show an error and stop */
void showFatal(const char* buf){
  onscreenNoticeMessage[0] = '\0';
  strncpy(onscreenNoticeMessage, buf, sizeof(onscreenNoticeMessage));
  Serial.print(F("FATAL ERROR:"));
  Serial.println(onscreenNoticeMessage);
  changeState(STATE_FATAL);
}

/* Show a notice and continue */
void showNotice(const char* buf){
  onscreenNoticeMessage[0] = '\0';
  strncpy(onscreenNoticeMessage, buf, sizeof(onscreenNoticeMessage));
  Serial.println(onscreenNoticeMessage);
  changeState(STATE_NOTICE);
}

/* Operate the OLED display */
void draw() {
  switch (displayState) {
    case STATE_FATAL:
      u8g.setColorIndex(1);
      u8g.setFont(u8g_font_6x10r);
      u8g.drawFrame(0, 0, DISPLAY_WIDTH, 34);
      u8g.drawStr( 28, 12, F("FATAL ERROR:"));
      u8g.drawStr( 6, 24, onscreenNoticeMessage);
      break;
    case STATE_NOTICE:
      u8g.setColorIndex(1);
      u8g.setFont(u8g_font_6x10r);
      u8g.drawFrame(0, 0, DISPLAY_WIDTH, 34);
      u8g.drawStr( 40, 12, F("Notice:"));
      u8g.drawStr( 6, 24, onscreenNoticeMessage);
      break;
    case STATE_SPLASH:
      u8g.setColorIndex(1);
      u8g.setFont(u8g_font_6x10r);
      u8g.drawStr( 36, 26, F("PHANTOM"));
      u8g.drawStr( 16, 38, F("FULL-THROTTLE"));
      u8g.drawStr( 16, 50,  F("SUPERCHARGER"));
      break;
    case STATE_NORMAL:
      u8g.setColorIndex(1);
      u8g.setFont(u8g_font_6x10r);
      u8g.setPrintPos(2, 10);
      u8g.print(F("MOT "));
      u8g.print(motorTempC, 1);
      u8g.print(F("C   CPU "));
      u8g.print(controllerTempC, 1);
      u8g.print(F("C"));
      if (isBoosting()) {
        u8g.setPrintPos(40, 20);
        u8g.print(F("Boosting"));
      } else { 
        u8g.setPrintPos(10, 20);     
        u8g.print(F("Last Boost: "));
        u8g.print(getLastBoostDurationMillis()/1000.0, 2);
        u8g.print(F(" s"));
      }
      
      for(int i=0; i<BARGRAPH_DATA_SZ; i++) {
        int h = getBarGraphDataPointInPast(i);
        int x = i*2;
        int yStart = DISPLAY_HEIGHT - h;
        u8g.drawVLine(DISPLAY_WIDTH-x, yStart, h);
        u8g.drawVLine(DISPLAY_WIDTH-x+1, yStart, h);        
      }
      

      u8g.setColorIndex(0);
      u8g.drawRBox(30, 36, 72, 28, 2);

      u8g.setColorIndex(1);
      u8g.drawRFrame(30, 36, 72, 28, 2);      

      u8g.setFont(u8g_font_helvB24n);
      u8g.setPrintPos(34, 63);      
      u8g.print(sensorVoltOne, 1);
      
      break;   
  }
}


void setup()   {       
  Serial.begin(9600);
  
  u8g.setHardwareBackup(u8g_backup_avr_spi);
  
  pinMode(SD_CS, OUTPUT);
  pinMode(OLED_CS, OUTPUT);
  
  changeState(STATE_SPLASH);

  // Setup bar graph
  setupBarGraph();
}

void loopNormal() {
  readVoltSensors();
  
  long time = millis();
  
  if (time - TEMP_READ_INTERVAL_MS > lastTempUpdate) {
    lastTempUpdate = time;
    /* Read the most recent values */
    readTempSensors();
    /* Let them process in the background */
    requestTemps();
  }
  
  if (time - DISK_WRITE_INTERVAL_MS > lastLogUpdate) {
    lastLogUpdate = time;
    
    /* Write to disk */
    logData(sensorVoltOne, motorTempC, controllerTempC);

    /* Debugging */
    Serial.print(F("Free RAM ="));
    Serial.println(freeMemory());

  }

  // Update the boost info
  updateBoostDuration();  
}

void loop() {
  
  /* Draw */
  u8g.firstPage();  
  do {
    draw();
  } while( u8g.nextPage() );
  
  /* State updates */
  switch(displayState){
    case STATE_SPLASH:
      /* Move to Normal. */
      changeState(STATE_NORMAL);
      
      /* Initialize the data logger */
      DataLogging_Begin(SD_CS, SD_MOSI, SD_MISO, SD_CLK);
      
      /* Set up the temperature probes */
      configureTempResolution();
      
      /* Request the first temperatures */
      requestTemps();
      
      /* Remain in splash for 2 seconds */
      delay(2000);
      
      break;
    case STATE_FATAL:
      while(1) {
        // Hold here forever
      }
      break;
    case STATE_NOTICE:
      /* Return to Normal. */
      changeState(STATE_NORMAL);
      
      /* Show notices for 4 seconds */
      delay(4000);      
      
      break;
    case STATE_NORMAL:
      loopNormal();    
      break;
   }
  
  delay(CYCLE_DELAY);
}



