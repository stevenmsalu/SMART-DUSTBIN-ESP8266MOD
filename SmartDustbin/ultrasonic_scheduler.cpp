#include <Arduino.h>

#include "approach_sensor.h"
#include "config.h"
#include "ultrasonic_scheduler.h"
#include "waste_sensor.h"

static unsigned long s_quietUntilMs = 0;
static unsigned long s_nextApproachMs = 0;
static unsigned long s_nextWasteMs = 0;
static bool s_serveWasteNext = false;

void ultrasonicSchedulerBegin() {
  s_quietUntilMs = 0;
  s_nextApproachMs = 0;
  s_nextWasteMs = 0;
  s_serveWasteNext = false;
}

UltrasonicMeasurementKind ultrasonicSchedulerTick(unsigned long nowMs) {
  if (nowMs < s_quietUntilMs) {
    return UltrasonicMeasurementKind::None;
  }

  const bool approachDue = (nowMs >= s_nextApproachMs);
  const bool wasteDue = (nowMs >= s_nextWasteMs);

  if (!approachDue && !wasteDue) {
    return UltrasonicMeasurementKind::None;
  }

  UltrasonicMeasurementKind kind = UltrasonicMeasurementKind::None;

  if (approachDue && wasteDue) {
    kind = s_serveWasteNext ? UltrasonicMeasurementKind::Waste
                            : UltrasonicMeasurementKind::Approach;
    s_serveWasteNext = !s_serveWasteNext;
  } else if (approachDue) {
    kind = UltrasonicMeasurementKind::Approach;
  } else {
    kind = UltrasonicMeasurementKind::Waste;
  }

  if (kind == UltrasonicMeasurementKind::Approach) {
    approachSensorRunMeasurement();
    s_nextApproachMs = millis() + CONFIG_APPROACH_SAMPLE_MS;
  } else {
    wasteSensorRunMeasurement();
    s_nextWasteMs = millis() + CONFIG_WASTE_SAMPLE_MS;
  }

  s_quietUntilMs = millis() + CONFIG_ULTRASONIC_QUIET_MS;
  return kind;
}
