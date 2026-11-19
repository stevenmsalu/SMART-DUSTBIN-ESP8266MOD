#ifndef SMART_DUSTBIN_APPLICATION_STATUS_H
#define SMART_DUSTBIN_APPLICATION_STATUS_H

#include <Arduino.h>

#include "lid_control.h"

// Unified snapshot for HTTP consumption (no heap allocation).
struct ApplicationStatusSnapshot {
  uint32_t uptimeMs;

  bool approachDetected;

  LidPositionState lidState;
  bool lidCloseDelayPending;
  uint8_t servoTargetAngleDeg;

  bool whiteLedOn;
};

void applicationStatusGetSnapshot(unsigned long nowMs,
                                  ApplicationStatusSnapshot& out);

void applicationStatusPrintDiagnostic(const ApplicationStatusSnapshot& snap);

#endif