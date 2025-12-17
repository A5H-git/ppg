#include <Wire.h>
#include <WiFi.h>
#include "heartMonitor.h"
#include "webHandler.h"


#define debug Serial //Uncomment this line if you're using an Uno or ESP

void printSensorValues(long irVal, float beatsPerMintue, float avgBPM)  {

  Serial.print("IR=");
  Serial.print(irVal);  
  Serial.print(", BPM=");
  Serial.print(beatsPerMintue);
  Serial.print(", Avg BPM=");
  Serial.println(avgBPM);
  Serial.println();
} 

void setup()
{
  Serial.begin(115200);

  // Initialize webserver
  bool webserverInitialized = initializeWebServer();
  if (!webserverInitialized) {
    Serial.println("Web Server Initialization Failed");
    return;
  }

  // Initialize Heart Monitor
  bool monitorInitialized = initializeHeartMonitor();
  if (!monitorInitialized) {
    Serial.println("Heart Monitor Initialization Failed");
    return;
  }

}

void loop() {
  // Heart Monitor
  updateSensorValues();
  // HeartMonitorMeasurements measurements = getMeasurements();

  // printSensorValues(measurements.irValue, measurements.beatsPerMinute, measurements.averageBPM);
  
}

