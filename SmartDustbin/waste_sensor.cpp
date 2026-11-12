#include <Arduino.h>

#include "config.h"
#include "hcsr04_common.h"
#include "pins.h"
#include "waste_sensor.h"

static WasteSensorReading s_last = {};
static bool s_hasLastValid = false;
static uint16_t s_lastValidCm = 0;

void wasteSensorBegin() {
  s_last = {};
  s_hasLastValid = false;
  s_lastValidCm = 0;
}

static void assignReading(WasteSensorReading& dst, const Hcsr04Reading& src) {
  dst.valid = src.valid;
  dst.timedOut = src.timedOut;
  dst.outOfRange = src.outOfRange;
  dst.distanceCm = src.distanceCm;
}

void wasteSensorRunMeasurement() {
  const Hcsr04Reading raw = hcsr04Measure(
      PIN_WASTE_TRIG, PIN_WASTE_ECHO, CONFIG_WASTE_PULSE_TIMEOUT_US,
      CONFIG_WASTE_MIN_VALID_CM, CONFIG_WASTE_MAX_VALID_CM,
      CONFIG_HCSR04_US_PER_CM);
  assignReading(s_last, raw);

  if (s_last.valid) {
    s_lastValidCm = s_last.distanceCm;
    s_hasLastValid = true;
  }
}

void wasteSensorGetReading(WasteSensorReading& out) {
  out = s_last;
}

bool wasteSensorGetLastValidDistance(uint16_t& distanceCm) {
  if (!s_hasLastValid) {
    return false;
  }
  distanceCm = s_lastValidCm;
  return true;
}
