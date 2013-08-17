#include <SD.h>
#include <Wire.h>
#include <RTClib.h>

RTC_DS1307 *RTC = NULL;
File dataFile;

boolean DataLogging_OpenFile(char* buf) {  
  char buf2[5];

  for (int i=0; i<9999; i++) {
    buf[0]='\0';
    buf2[0]='\0';
    itoa(i, buf2, 10);

    strcat(buf, "log-");
    strcat(buf, buf2);
    strcat(buf, ".csv");

    if (!SD.exists(buf)) {
      dataFile = SD.open(buf, FILE_WRITE);
      return true;
    }
  }
  return false;
}

void DataLogging_Begin(int pin_cs, int pin_mosi, int pin_miso, int pin_clk) {
  Wire.begin();
  RTC = new RTC_DS1307();
  RTC->begin();

  if (! RTC->isrunning()) {
    Serial.println(F("RTC is not running!"));
    // following line sets the RTC to the date & time this sketch was compiled
    // uncomment it & upload to set the time, date and start run the RTC!
    //    RTC.adjust(DateTime(__DATE__, __TIME__));
  } 
  else {
    DateTime now = RTC->now();
    Serial.print(F("RTC operating acceptably. Date is "));
    Serial.print(now.year(), DEC);
//    Serial.print('/');
    Serial.print(now.month(), DEC);
//    Serial.print('/');
    Serial.print(now.day(), DEC);
//    Serial.print(' ');
    Serial.print(now.hour(), DEC);
//    Serial.print(':');
    Serial.print(now.minute(), DEC);
//    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();    
  }      

  // see if the card is present and can be initialized:
  if (!SD.begin(pin_cs, pin_mosi, pin_miso, pin_clk)) {
//    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1) ;
  }
//  Serial.println("Card initialized."); 

  // Open up the file we're going to log to!  
  char buffer[13];
  buffer[0] = '\0';

  if (DataLogging_OpenFile(buffer)) {
    Serial.print(F("Opened datalog at "));
    Serial.println(buffer);
    
  } else {
    Serial.print(F("Error opening datalog at "));
    Serial.println(buffer);
    
    // Wait forever since we cant write data
    while (1) ;
  }    
}

void logData(float sensorVoltOne, float sensorTempOneC, float sensorTempTwoC) {
   dataFile.print(millis());
   dataFile.print(",");
   dataFile.print(sensorVoltOne, 2);
   dataFile.print(",");
   dataFile.print(sensorTempOneC, 2);
   dataFile.print(",");
   dataFile.print(sensorTempTwoC, 2);
   dataFile.println();
   dataFile.flush();
}
