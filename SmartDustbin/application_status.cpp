#include <Arduino.h>

#include "application_status.h"
#include "approach_sensor.h"
#include "white_indicator.h"

void applicationStatusGetSnapshot(unsigned long nowMs,
                                  ApplicationStatusSnapshot& out) {
  out.uptimeMs = static_cast<uint32_t>(nowMs);
  out.approachDetected = approachSensorIsApproachDetected();

  out.lidState = lidControlGetLidState();
  out.lidCloseDelayPending = lidControlIsCloseDelayPending();
  out.servoTargetAngleDeg = lidControlGetServoTargetAngle();

  out.whiteLedOn = whiteIndicatorIsOn();
}

static const __FlashStringHelper* lidLabel(LidPositionState st) {
  return (st == LidPositionState::Open) ? F("OPEN") : F("CLOSED");
}

void applicationStatusPrintDiagnostic(const ApplicationStatusSnapshot& snap) {
  Serial.print(F("[app] uptime_ms="));
  Serial.print(snap.uptimeMs);
  Serial.print(F(" approach="));
  Serial.print(snap.approachDetected ? 1 : 0);

  Serial.print(F(" lid="));
  Serial.print(lidLabel(snap.lidState));
  Serial.print(F(" close_pending="));
  Serial.print(snap.lidCloseDelayPending ? 1 : 0);

  Serial.print(F(" white="));
  Serial.print(snap.whiteLedOn ? 1 : 0);

  Serial.println();
}