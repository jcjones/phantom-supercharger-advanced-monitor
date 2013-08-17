#include "Globals.h"

#define GRAPH_HEIGHT 40

void addBarGraphDataPoint(int value) {
//  barGraphIndex += 1;
//  if (barGraphIndex > display.width()){
//    barGraphIndex = 0;
//  }
//
//  barGraphData[barGraphIndex] = map(value, 0, 1023, 0, GRAPH_HEIGHT);

  //  Serial.print("Updating ");
  //  Serial.print(barGraphCurrent);
  //  Serial.print(" with ");
  //  Serial.print(value);
  //  Serial.print(" which is ");
  //  Serial.println(barGraphData[barGraphCurrent]);
}

int getBarGraphDataPointInPast(int cycleOffset) {
  return barGraphData[(barGraphIndex - cycleOffset) % BARGRAPH_DATA_SZ];
}

void drawBars() {
//  for(uint8_t i = 0; i < display.width(); i++) {
//    int barHeight = getBarGraphDataPointInPast(i);//barGraphData[(barGraphIndex - i) % BARGRAPH_DATA_SZ];
//    //    Serial.print(i);
//    //    Serial.print(" = ");
//    //    Serial.println(barHeight);
//
//    display.drawFastVLine(display.width()-i, display.height()-barHeight, barHeight, WHITE);
//  }
}


