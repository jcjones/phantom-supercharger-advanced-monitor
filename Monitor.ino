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
#define RELAY 4

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
long stateEndTimeMs = 0;

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
Butt| S-Orange         | 2 (button)
*/

/* Screen */
U8GLIB_SSD1306_128X64 u8g(OLED_CLK, OLED_MOSI, OLED_CS, OLED_DC, OLED_RESET); // HW SPI Com: CS = 10, A0 = 9 (Hardware Pins are  SCK = 13 and MOSI = 11)

float controllerTempC;
float motorTempC;
float sensorVoltOne;
int sensorVoltOneI;

int lastButtonState = LOW;


// Used by the Fatal state
char onscreenNoticeMessage[33];

#define STATE_SPLASH 0
#define STATE_COUNTDOWN 1
#define STATE_NOTICE 8
#define STATE_FATAL 9
#define STATE_NORMAL 2
#define STATE_DISABLED 3
int displayState;


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
//  sensorVoltOneI = (int)(292.0*sin(millis()/5000.0)+732.0);

  Serial.print(F("Read Val: "));
  Serial.print(sensorVoltOneI);
  Serial.print(F("  Conv: "));
  Serial.println(sensorVoltOne);

  sensorVoltOne = sensorVoltOneI * (30.0 / 1024.0);

  addBarGraphDataPoint(sensorVoltOneI);
}

/* Only trigger on a state change to HIGH */
boolean readButtonState() {
  boolean result = false;
  int buttonState = digitalRead(BUTTON);
  
  if (HIGH == buttonState && LOW == lastButtonState) {
    result = true;
  }
  
  lastButtonState = buttonState;
  
  return result;
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


/* Do state updates for Normal and Disabled states */
void loopNormalAndDisabled() {
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
      u8g.drawFrame(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
      u8g.drawStr( 41, 12, F("Notice:"));
      u8g.drawStr( 6, 24, onscreenNoticeMessage);
      u8g.drawStr( 26, 61, F("Press button."));      
      break;
    case STATE_SPLASH:
      u8g.setColorIndex(1);
      u8g.setFont(u8g_font_6x10r);
      u8g.drawStr( 27, 22, F("   PHANTOM   "));
      u8g.drawStr( 27, 34, F("FULL-THROTTLE"));
      u8g.drawStr( 29, 46,  F("SUPERCHARGER"));
      break;
    case STATE_COUNTDOWN:
      u8g.setFont(u8g_font_6x10r);
      u8g.drawStr( 26, 22, F("Starting in:"));
      
      u8g.setFont(u8g_font_helvB24n);
      u8g.setPrintPos(40, 63);      
      {
        long timeLeftMillis = stateEndTimeMs - millis();
        float timeLeftSeconds = 0.0;
        /* Print 0.0 if the remaining millis is not positive */
        if (timeLeftMillis > 0) {
          timeLeftSeconds = (timeLeftMillis) / 1000.0;        
        }
        u8g.print(timeLeftSeconds, 1);        
      }
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
//        u8g.print(" ");
//        u8g.print(sensorVoltOneI);
            
      break;   
case STATE_DISABLED:
      u8g.setColorIndex(1);
      u8g.setFont(u8g_font_6x10r);
      u8g.setPrintPos(2, 10);
      u8g.print(F("MOT "));
      u8g.print(motorTempC, 1);
      u8g.print(F("C   CPU "));
      u8g.print(controllerTempC, 1);
      u8g.print(F("C"));
     
      u8g.setColorIndex(1);      
      u8g.drawStr( 20, 61, F("Press to Enable"));      
      break;         
  }
}


void setup()   {       
  Serial.begin(9600);
  
  u8g.setHardwareBackup(u8g_backup_avr_spi);
  
  pinMode(SD_CS, OUTPUT);
  pinMode(OLED_CS, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(BUTTON, INPUT);
   
  changeState(STATE_SPLASH);

  // Setup bar graph
  setupBarGraph();
}


/* State change function */
void changeState(int newState) {
//  Serial.print(F("S_OLD: "));
//  Serial.print(displayState);  
//  Serial.print(F("   S_NEW: "));
//  Serial.println(newState);  
  displayState = newState;
  
  switch (displayState) {
    case STATE_NORMAL:
      Serial.println(F("Relay enabled"));
      digitalWrite(RELAY, HIGH);   
      break;    
    case STATE_DISABLED:
      Serial.println(F("Relay disabled"));
      digitalWrite(RELAY, LOW);    
      break;
    case STATE_COUNTDOWN:
      stateEndTimeMs = millis() + COUNTDOWN_TIME_MS;
      break;
  }
}


void loop() {
  
  /* Draw */
  u8g.firstPage();  
  do {
    draw();
  } while( u8g.nextPage() );

  /* Load button state */
  boolean buttonPressed = readButtonState();
  
 
  /* State updates */
  switch(displayState){
    case STATE_SPLASH:    
      /* Initialize the data logger */
      DataLogging_Begin(SD_CS, SD_MOSI, SD_MISO, SD_CLK);
      
      /* Set up the temperature probes */
      configureTempResolution();
      
      /* Request the first temperatures */
      requestTemps();
      
      /* Remain in splash for 2 seconds */
      delay(2000);
      
      /* Move to Countdown. */
      changeState(STATE_COUNTDOWN);     
      
      break;
    case STATE_FATAL:
      while(1) {
        // Hold here forever
      }
      break;
    case STATE_NOTICE:
      if (buttonPressed) {
        /* Return to Normal. */
        changeState(STATE_NORMAL);
      }
      
      break;
    case STATE_COUNTDOWN:
      if (stateEndTimeMs < millis() || buttonPressed) {
        /* Return to Normal. */
        changeState(STATE_NORMAL);
      }
            
      break;
    case STATE_NORMAL:
      if (buttonPressed) {
        changeState(STATE_DISABLED);        
      }
      
      loopNormalAndDisabled();
      break;
    case STATE_DISABLED:
      if (buttonPressed) {
        changeState(STATE_NORMAL);        
      }
    
      loopNormalAndDisabled();
      break;
   }
  
  delay(CYCLE_DELAY);
}



