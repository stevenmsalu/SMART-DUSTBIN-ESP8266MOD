#include <Arduino.h>

#include "application_status.h"
#include "approach_sensor.h"

void applicationStatusGetSnapshot(unsigned long nowMs,
                                  ApplicationStatusSnapshot& out) {
  out.uptimeMs = static_cast<uint32_t>(nowMs);
  out.approachDetected = approachSensorIsApproachDetected();

  // WP-11.1: waste subsystem removed; API fields left unavailable until WP-11.3.
  out.hasWasteDistanceLastValid = false;
  out.wasteDistanceLastValidCm = 0;
  out.wasteLatestAttemptValid = false;
  out.wasteLatestTimedOut = false;
  out.wasteLatestOutOfRange = false;
  out.hasFillPercentLastValid = false;
  out.fillPercentLastValid = 0;

  out.lidState = lidControlGetLidState();
  out.lidCloseDelayPending = lidControlIsCloseDelayPending();
  out.servoTargetAngleDeg = lidControlGetServoTargetAngle();

  out.ledIndicatorState = ledStatusGetState();
  out.whiteLedOn = ledStatusGetWhiteOn();
  out.yellowLedOn = ledStatusGetYellowOn();
}

static const __FlashStringHelper* lidLabel(LidPositionState st) {
  return (st == LidPositionState::Open) ? F("OPEN") : F("CLOSED");
}

static const __FlashStringHelper* ledLabel(LedFillIndicatorState st) {
  switch (st) {
    case LedFillIndicatorState::Normal:
      return F("NORMAL");
    case LedFillIndicatorState::Warning:
      return F("WARNING");
    default:
      return F("UNKNOWN");
  }
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

  Serial.print(F(" led="));
  Serial.print(ledLabel(snap.ledIndicatorState));
  Serial.print(F(" white="));
  Serial.print(snap.whiteLedOn ? 1 : 0);
  Serial.print(F(" yellow="));
  Serial.print(snap.yellowLedOn ? 1 : 0);

  Serial.println();
}