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

// Call each loop; runs a measurement when the sample interval has elapsed.
// Returns true when a new reading was taken (valid or invalid).
bool approachSensorUpdate(unsigned long nowMs);

void approachSensorGetReading(ApproachSensorReading& out);

bool approachSensorIsApproachDetected();

#endif
