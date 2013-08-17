#ifndef Globals_h
#define Globals_h

#include "Arduino.h"

#define BARGRAPH_DATA_SZ 64
uint8_t barGraphData[BARGRAPH_DATA_SZ];
uint8_t barGraphIndex = 0;

#define CYCLE_DELAY 250

#endif // Globals_h
