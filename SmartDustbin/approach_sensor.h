#ifndef SMART_DUSTBIN_APPROACH_SENSOR_H
#define SMART_DUSTBIN_APPROACH_SENSOR_H

#include <Arduino.h>

struct ApproachSensorReading {
  bool valid;
  bool timedOut;
  bool outOfRange;
  uint16_t distanceCm;
};

void approachSensorBegin();

// Invoked only by ultrasonic_scheduler (one HC-SR04 active at a time).
void approachSensorRunMeasurement();

void approachSensorGetReading(ApproachSensorReading& out);

bool approachSensorIsApproachDetected();

#endif
