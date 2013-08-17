#include "Globals.h"


static long lastBoostStartMs = 0;
static long lastBoostEndMs = 0;
static long thisBoostStartMs = 0;
static boolean boosting = false;

int getLastBoostDurationMillis() {
  if ((0 == lastBoostStartMs) || (0 == lastBoostEndMs)) {
    return 0;
  }
  
  return lastBoostEndMs - lastBoostStartMs;
}

int updateBoostDuration() {
  float dxdt = (float)(getBarGraphDataPointInPast(0) - getBarGraphDataPointInPast(2)) / (CYCLE_DELAY * 3.0); // 3 ticks total
  
//  Serial.print("Boost ");
//  Serial.print(dxdt,2);
//  Serial.print(" stat ");
//  Serial.print(boosting?"t":"f");
//  Serial.println("");
  
  if (dxdt >= 0.0) {
    if (boosting) {
      lastBoostStartMs = thisBoostStartMs;
      lastBoostEndMs = millis();
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

