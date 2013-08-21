#include "Globals.h"


static long lastBoostMs = 0;
static long thisBoostStartMs = 0;
static boolean boosting = false;

int getLastBoostDurationMillis() {
  return lastBoostMs;
}

inline float getdVdT() {
  return (float)(getBarGraphDataPointInPast(0) - getBarGraphDataPointInPast(4)) / (CYCLE_DELAY * 5.0); // 5 ticks total
}

int updateBoostDuration() {
  float dvdt = getdVdT();
  
//  Serial.print("Boost ");
//  Serial.print(dxdt,2);
//  Serial.print(" stat ");
//  Serial.print(boosting?"t":"f");
//  Serial.println("");
  
  if (dvdt >= 0.0) {
    if (boosting) {
      lastBoostMs = millis() - thisBoostStartMs;
      boosting = false;
    }
  } else {
    if (!boosting) {
      thisBoostStartMs = millis();
      boosting = true;
    }
  }    
}

boolean isBoosting() {
  return boosting;
}

