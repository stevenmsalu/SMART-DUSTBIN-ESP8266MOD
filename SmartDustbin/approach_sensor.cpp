#include <Arduino.h>

#include "approach_sensor.h"
#include "config.h"
#include "pins.h"

static unsigned long s_lastSampleMs = 0;
static ApproachSensorReading s_last = {};
static bool s_approachDetected = false;

void approachSensorBegin() {
  s_lastSampleMs = 0;
  s_last = {};
  s_approachDetected = false;
}

static ApproachSensorReading measureOnce() {
  ApproachSensorReading r = {};
  r.valid = false;
  r.timedOut = false;
  r.outOfRange = false;
  r.distanceCm = 0;

  digitalWrite(PIN_APPROACH_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_APPROACH_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_APPROACH_TRIG, LOW);

  const unsigned long durationUs =
      pulseIn(PIN_APPROACH_ECHO, HIGH, CONFIG_APPROACH_PULSE_TIMEOUT_US);

  if (durationUs == 0) {
    r.timedOut = true;
    return r;
  }

  const uint32_t distanceCm =
      static_cast<uint32_t>(durationUs / CONFIG_APPROACH_US_PER_CM);

  if (distanceCm < CONFIG_APPROACH_MIN_VALID_CM ||
      distanceCm > CONFIG_APPROACH_MAX_VALID_CM) {
    r.outOfRange = true;
    r.distanceCm = static_cast<uint16_t>(distanceCm);
    return r;
  }

  r.valid = true;
  r.distanceCm = static_cast<uint16_t>(distanceCm);
  return r;
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

bool approachSensorUpdate(unsigned long nowMs) {
  if (s_lastSampleMs != 0 &&
      (nowMs - s_lastSampleMs) < CONFIG_APPROACH_SAMPLE_MS) {
    return false;
  }
  s_lastSampleMs = nowMs;

  s_last = measureOnce();
  updateApproachState(s_last);
  return true;
}

void approachSensorGetReading(ApproachSensorReading& out) {
  out = s_last;
}

bool approachSensorIsApproachDetected() {
  return s_approachDetected;
}
