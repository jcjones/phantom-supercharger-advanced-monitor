#ifndef Globals_h
#define Globals_h

#include "Arduino.h"

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

#define GRAPH_HEIGHT 40
#define GRAPH_WIDTH BARGRAPH_DATA_SZ

#define BARGRAPH_DATA_SZ 64

#define CYCLE_DELAY 100
#define DISK_WRITE_INTERVAL_MS 1000
#define TEMP_READ_INTERVAL_MS 1000

#define ONEWIRE_POWERED_POLLTIME_MS 100

// OneWire Commands
#define STARTCONVO      0x44  // Tells device to take a temperature reading and put it on the scratchpad
#define READSCRATCH     0xBE  // Read EEPROM
#define WRITESCRATCH    0x4E  // Write to EEPROM
#define COPYSCRATCH     0x48  // Copy EEPROM

// OneWire Configurations
#define TEMP_12_BIT 0x7F // 12 bit

// OneWire Scratchpad locations
#define TEMP_LSB        0
#define TEMP_MSB        1
#define HIGH_ALARM_TEMP 2
#define LOW_ALARM_TEMP  3
#define CONFIGURATION   4
#define INTERNAL_BYTE   5
#define COUNT_REMAIN    6
#define COUNT_PER_C     7
#define SCRATCHPAD_CRC  8

#define NOT_PARASITE  0

#endif // Globals_h
