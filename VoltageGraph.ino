#include "Globals.h"

#define GRAPH_HEIGHT ( SSD1306_LCDHEIGHT - SSD1306_BUFHEIGHT - 1 )

void addBarGraphDataPoint(int value) {
  barGraphIndex += 1;
  if (barGraphIndex > display.width()){
    barGraphIndex = 0;
  }

  barGraphData[barGraphIndex] = map(value, 0, 1023, 0, GRAPH_HEIGHT);

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

uint8_t getGraphPixels(int16_t x, int16_t y) {
  int barHeight = getBarGraphDataPointInPast(x);
  
  return barHeight;
}

void drawBars() {
//  for(uint8_t i = 0; i < display.width(); i++) {
//    
//    //    Serial.print(i);
//    //    Serial.print(" = ");
//    //    Serial.println(barHeight);
//
//    display.drawFastVLine(display.width()-i, display.height()-barHeight, barHeight, WHITE);
//  }
}


