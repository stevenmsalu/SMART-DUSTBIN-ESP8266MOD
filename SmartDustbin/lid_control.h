#ifndef SMART_DUSTBIN_LID_CONTROL_H
#define SMART_DUSTBIN_LID_CONTROL_H

#include <Arduino.h>

enum class LidPositionState : uint8_t {
  Closed = 0,
  Open = 1,
};

void lidControlBegin();

// Non-blocking state machine; call every loop().
void lidControlUpdate(unsigned long nowMs, bool approachDetected);

LidPositionState lidControlGetLidState();

uint8_t lidControlGetServoTargetAngle();

bool lidControlIsCloseDelayPending();

#endif
