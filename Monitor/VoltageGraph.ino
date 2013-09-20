#include "Globals.h"

uint8_t barGraphData[BARGRAPH_DATA_SZ];
int barGraphIndex = 0;

void setupBarGraph() {
  barGraphIndex = 0;
  memset(barGraphData, 0, BARGRAPH_DATA_SZ);
}

void addBarGraphDataPoint(int value) {
  barGraphIndex += 1;
  if (barGraphIndex >= BARGRAPH_DATA_SZ){
    barGraphIndex = 0;
  }

  // From 16 V to 28.5 V, scaled
  value = constrain(value, 166, 285);
  barGraphData[barGraphIndex] = map(value, 160, 285, 0, GRAPH_HEIGHT);
}

int getBarGraphDataPointInPast(int cycleOffset) {
  if ((cycleOffset > BARGRAPH_DATA_SZ) || (cycleOffset < 0)) {
    showFatal("Invalid offset");
  }
  
  int idx = (BARGRAPH_DATA_SZ + (barGraphIndex - cycleOffset)) % BARGRAPH_DATA_SZ;
  return barGraphData[idx];
}

