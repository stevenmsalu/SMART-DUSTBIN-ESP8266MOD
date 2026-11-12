#ifndef SMART_DUSTBIN_WASTE_SENSOR_H
#define SMART_DUSTBIN_WASTE_SENSOR_H

#include <Arduino.h>

struct WasteSensorReading {
  bool valid;
  bool timedOut;
  bool outOfRange;
  uint16_t distanceCm;
};

void wasteSensorBegin();

// Invoked only by ultrasonic_scheduler (one HC-SR04 active at a time).
void wasteSensorRunMeasurement();

void wasteSensorGetReading(WasteSensorReading& out);

// Latest valid distance to waste surface (unchanged until next valid sample).
bool wasteSensorGetLastValidDistance(uint16_t& distanceCm);

#endif
