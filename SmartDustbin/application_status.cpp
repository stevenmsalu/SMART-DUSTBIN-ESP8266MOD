#include <Arduino.h>

#include "application_status.h"
#include "approach_sensor.h"
#include "waste_fill_level.h"
#include "waste_sensor.h"

void applicationStatusGetSnapshot(unsigned long nowMs,
                                  ApplicationStatusSnapshot& out) {
  out.uptimeMs = static_cast<uint32_t>(nowMs);
  out.approachDetected = approachSensorIsApproachDetected();

  out.hasWasteDistanceLastValid =
      wasteSensorGetLastValidDistance(out.wasteDistanceLastValidCm);

  WasteSensorReading wasteLatest;
  wasteSensorGetReading(wasteLatest);
  out.wasteLatestAttemptValid = wasteLatest.valid;
  out.wasteLatestTimedOut = wasteLatest.timedOut;
  out.wasteLatestOutOfRange = wasteLatest.outOfRange;

  out.hasFillPercentLastValid =
      wasteFillLevelGetLastValidPercent(out.fillPercentLastValid);

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

  Serial.print(F(" waste_last_valid_cm="));
  if (snap.hasWasteDistanceLastValid) {
    Serial.print(snap.wasteDistanceLastValidCm);
  } else {
    Serial.print(F("none"));
  }

  Serial.print(F(" waste_latest_valid="));
  Serial.print(snap.wasteLatestAttemptValid ? 1 : 0);
  if (!snap.wasteLatestAttemptValid) {
    Serial.print(F(" latest_invalid_reason="));
    if (snap.wasteLatestTimedOut) {
      Serial.print(F("timeout"));
    } else if (snap.wasteLatestOutOfRange) {
      Serial.print(F("out_of_range"));
    } else {
      Serial.print(F("none"));
    }
  }

  Serial.print(F(" fill_last_valid_pct="));
  if (snap.hasFillPercentLastValid) {
    Serial.print(snap.fillPercentLastValid);
  } else {
    Serial.print(F("none"));
  }

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
