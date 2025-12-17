#include <Arduino.h>

struct HeartMonitorMeasurements {
  unsigned long timestamp;
  long irValue;
  float beatsPerMinute;
  float averageBPM;
};

// Function Prototypes
void updateSensorValues();
bool initializeHeartMonitor();
HeartMonitorMeasurements getMeasurements();
size_t drainMeasurements(HeartMonitorMeasurements* dest, size_t maxCount);
