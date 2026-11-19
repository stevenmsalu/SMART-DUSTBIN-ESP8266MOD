#include <Arduino.h>

#include "approach_sensor.h"
#include "../config.h"
#include "hcsr04_common.h"
#include "../pins.h"

static ApproachSensorReading s_last = {};
static bool s_approachDetected = false;

void approachSensorBegin() {
  s_last = {};
  s_approachDetected = false;
}

static void updateApproachState(const ApproachSensorReading& r) {
  if (!r.valid) {
    return;
  }
  if (r.distanceCm <= CONFIG_APPROACH_DETECT_CM) {
    s_approachDetected = true;
  } else if (r.distanceCm >= CONFIG_APPROACH_CLEAR_CM) {
    s_approachDetected = false;
  }
}

static void assignReading(ApproachSensorReading& dst,
                          const Hcsr04Reading& src) {
  dst.valid = src.valid;
  dst.timedOut = src.timedOut;
  dst.outOfRange = src.outOfRange;
  dst.distanceCm = src.distanceCm;
}

void approachSensorRunMeasurement() {
  const Hcsr04Reading raw = hcsr04Measure(
      PIN_APPROACH_TRIG, PIN_APPROACH_ECHO, CONFIG_APPROACH_PULSE_TIMEOUT_US,
      CONFIG_APPROACH_MIN_VALID_CM, CONFIG_APPROACH_MAX_VALID_CM,
      CONFIG_HCSR04_US_PER_CM);
  assignReading(s_last, raw);
  updateApproachState(s_last);
}

void approachSensorGetReading(ApproachSensorReading& out) {
  out = s_last;
}

bool approachSensorIsApproachDetected() {
  return s_approachDetected;
}
