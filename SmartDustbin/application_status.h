#ifndef SMART_DUSTBIN_APPLICATION_STATUS_H
#define SMART_DUSTBIN_APPLICATION_STATUS_H

#include <Arduino.h>

#include "led_status.h"
#include "lid_control.h"

// Unified snapshot for WP-8 HTTP consumption (no heap allocation).
struct ApplicationStatusSnapshot {
  uint32_t uptimeMs;

  bool approachDetected;

  bool hasWasteDistanceLastValid;
  uint16_t wasteDistanceLastValidCm;

  bool wasteLatestAttemptValid;
  bool wasteLatestTimedOut;
  bool wasteLatestOutOfRange;

  bool hasFillPercentLastValid;
  uint8_t fillPercentLastValid;

  LidPositionState lidState;
  bool lidCloseDelayPending;
  uint8_t servoTargetAngleDeg;

  LedFillIndicatorState ledIndicatorState;
  bool whiteLedOn;
  bool yellowLedOn;
};

void applicationStatusGetSnapshot(unsigned long nowMs,
                                  ApplicationStatusSnapshot& out);

void applicationStatusPrintDiagnostic(const ApplicationStatusSnapshot& snap);

#endif
