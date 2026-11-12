#ifndef SMART_DUSTBIN_ULTRASONIC_SCHEDULER_H
#define SMART_DUSTBIN_ULTRASONIC_SCHEDULER_H

#include <Arduino.h>

enum class UltrasonicMeasurementKind : uint8_t {
  None = 0,
  Approach = 1,
  Waste = 2,
};

void ultrasonicSchedulerBegin();

// Runs at most one coordinated measurement per call when due and acoustic quiet allows.
// Returns the sensor that was measured, or None.
UltrasonicMeasurementKind ultrasonicSchedulerTick(unsigned long nowMs);

#endif
