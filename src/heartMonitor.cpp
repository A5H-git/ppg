#include <Wire.h>

#include "heartMonitor.h"
#include "heartRate.h"
#include "MAX30105.h"

static MAX30105 particleSensor;

/*=============
* Constants
*=============*/
static const int PRESENCE_THRESHOLD_IR = 100000; // Threshold for detecting presence
static const byte AVG_SAMPLING_WINDOW_SIZE = 4; // Size of the averaging window
static const long IR_JUMP_THRESHOLD = 1000; // Max allowed change between samples

// Sensor Configuration
static const byte SDA_PIN = 21;
static const byte SCL_PIN = 22;
static const byte MIN_VALID_BPM = 20;
static const byte MAX_VALID_BPM = 255;
static const byte MAX_SAMPLES_PER_CALL = 4;

static const byte LED_BRIGHTNESS = 0x1F; // datasheet
static const byte SAMPLE_AVG = 4;
static const byte LED_MODE = 2; // red + IR
static const int SAMPLE_RATE = 400;
static const int PULSE_WIDTH = 411;
static const int ADC_RANGE = 4096;

// Circular buffer for BPM samples
static float tSamples[AVG_SAMPLING_WINDOW_SIZE];
static byte tIndex = 0;


// Variables
static long irValue = 0;
static long lastIRValue = 0;
static bool hasLastIR = false;
static float beatsPerMinute = 0.0f;
static float averageBPM = 0.0f;
static long lastBeat = 0;
static unsigned long lastMeasurementTimestamp = 0;

// Measurement queue
static const size_t MEASUREMENT_QUEUE_SIZE = 64;
static HeartMonitorMeasurements measurementQueue[MEASUREMENT_QUEUE_SIZE];
static size_t measurementHead = 0;
static size_t measurementTail = 0;
static size_t measurementCount = 0;

/*================
* Private Functions
*================*/
static float calculateAverageBPM() {  float total = 0.0f;

  for (int i = 0; i < AVG_SAMPLING_WINDOW_SIZE; i++) {
    total += tSamples[i];
  }

  return (float)total / AVG_SAMPLING_WINDOW_SIZE;
}


static float updateAverageBPM(float newBPM) {
  tSamples[tIndex++] = newBPM;
  tIndex %= AVG_SAMPLING_WINDOW_SIZE; //Wrap variable for circular buffer
  averageBPM = calculateAverageBPM();
  return averageBPM;
}

static bool fingerPresent(long irValue) {
  return irValue > PRESENCE_THRESHOLD_IR;
}


static void resetBPMData() {
  beatsPerMinute = 0;
  averageBPM = 0;
  tIndex = 0;

  for (int i = 0; i < AVG_SAMPLING_WINDOW_SIZE; i++) {
    tSamples[i] = 0;
  }
}

static void enqueueMeasurement() {
  HeartMonitorMeasurements measurement;
  measurement.timestamp = lastMeasurementTimestamp;
  measurement.irValue = irValue;
  measurement.beatsPerMinute = beatsPerMinute;
  measurement.averageBPM = averageBPM;

  measurementQueue[measurementHead] = measurement;
  measurementHead = (measurementHead + 1) % MEASUREMENT_QUEUE_SIZE;

  if (measurementCount == MEASUREMENT_QUEUE_SIZE) {
    measurementTail = (measurementTail + 1) % MEASUREMENT_QUEUE_SIZE;
  } else {
    measurementCount++;
  }
}

static bool isValidBpmMeasurement(float bpm) {
  bool bpmOk = (bpm >= MIN_VALID_BPM && bpm <= MAX_VALID_BPM);
  bool jumpOk = (!hasLastIR) || (labs(irValue - lastIRValue) <= IR_JUMP_THRESHOLD);
  return bpmOk && jumpOk;
}

static void processSample(long sample) {
  irValue = sample;
  lastMeasurementTimestamp = millis();

  if (!fingerPresent(irValue)) {
    resetBPMData();
    enqueueMeasurement();
    return;
  }

  if (checkForBeat(irValue)) {
    long now = millis();

    if (lastBeat != 0) {
      long dT = now - lastBeat; // ms between beats

      float bpm = 60000.0 / (float)dT;

      // Simple physiological BPM validation
      if (isValidBpmMeasurement(bpm)) {
        beatsPerMinute = bpm;
        updateAverageBPM(bpm);
      }
      
    }
    
    lastBeat = now;
  }

  enqueueMeasurement();
  lastIRValue = irValue;
  hasLastIR = true;
}

/*========================
* Public API Functions
*========================*/

HeartMonitorMeasurements getMeasurements() {
  HeartMonitorMeasurements measurements;
  measurements.timestamp = lastMeasurementTimestamp;
  measurements.irValue = irValue;
  measurements.beatsPerMinute = beatsPerMinute;
  measurements.averageBPM = averageBPM;
  return measurements;
}

size_t drainMeasurements(HeartMonitorMeasurements* dest, size_t maxCount) {
  size_t copied = 0;

  while (copied < maxCount && measurementCount > 0) {
    dest[copied++] = measurementQueue[measurementTail];
    measurementTail = (measurementTail + 1) % MEASUREMENT_QUEUE_SIZE;
    measurementCount--;
  }

  return copied;
}

bool initializeHeartMonitor() {
  // Initialize Hardware
  Wire.setPins(SDA_PIN, SCL_PIN); // ESP32 default I2C pins
  Wire.begin();

  if (!particleSensor.begin()) {
    Serial.println("Sensor not detected. Check power and wiring.");
    return false;
  }

  particleSensor.setup(
    LED_BRIGHTNESS,
    SAMPLE_AVG,
    LED_MODE,
    SAMPLE_RATE,
    PULSE_WIDTH,
    ADC_RANGE
  );

  // Set sensor intensities; green not needed
  particleSensor.setPulseAmplitudeRed(LED_BRIGHTNESS);
  particleSensor.setPulseAmplitudeIR(LED_BRIGHTNESS); // use default
  particleSensor.setPulseAmplitudeGreen(0);  
  return true;
}

void updateSensorValues() {
  particleSensor.check(); // load new data if available
  if (!particleSensor.available()) {
    return;
  }

  byte processed = 0;
  while (particleSensor.available() && processed < MAX_SAMPLES_PER_CALL) {
    long sample = particleSensor.getIR();
    particleSensor.nextSample();
    processSample(sample);
    processed++;
  }

  if (particleSensor.available()) {
    particleSensor.clearFIFO();
  }
}
