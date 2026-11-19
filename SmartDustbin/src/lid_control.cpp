#include <Arduino.h>
#include <Servo.h>

#include "../config.h"
#include "lid_control.h"
#include "../pins.h"

static Servo s_servo;
static LidPositionState s_lidState = LidPositionState::Closed;
static bool s_closeDelayPending = false;
static unsigned long s_closeDelayStartMs = 0;
static uint8_t s_committedAngle = 0;
static uint8_t s_targetAngle = 0;
static bool s_servoReady = false;

static uint8_t clampServoAngle(int angle) {
  if (angle < 0) {
    return 0;
  }
  if (angle > 180) {
    return 180;
  }
  return static_cast<uint8_t>(angle);
}

static bool servoAnglesConfigValid() {
  return CONFIG_SERVO_ANGLE_CLOSED >= 0 && CONFIG_SERVO_ANGLE_CLOSED <= 180 &&
         CONFIG_SERVO_ANGLE_OPEN >= 0 && CONFIG_SERVO_ANGLE_OPEN <= 180;
}

static void moveServoIfNeeded(uint8_t angle, const __FlashStringHelper* eventTag) {
  if (!s_servoReady) {
    return;
  }
  s_targetAngle = angle;
  if (s_committedAngle == angle) {
    return;
  }
  s_committedAngle = angle;
  s_servo.write(angle);
  Serial.print(eventTag);
  Serial.print(F(" target_angle="));
  Serial.println(angle);
}

static bool closeDelayElapsed(unsigned long nowMs) {
  return (nowMs - s_closeDelayStartMs) >= CONFIG_LID_CLOSE_DELAY_MS;
}

void lidControlBegin() {
  s_lidState = LidPositionState::Closed;
  s_closeDelayPending = false;
  s_closeDelayStartMs = 0;
  s_servoReady = false;

  if (!servoAnglesConfigValid()) {
    Serial.println(F("[lid] ERROR: servo angles must be 0-180"));
    return;
  }

  s_servo.attach(PIN_SERVO_SIGNAL);
  s_servoReady = true;

  const uint8_t closedAngle = clampServoAngle(CONFIG_SERVO_ANGLE_CLOSED);
  s_committedAngle = 0xFF;
  s_targetAngle = closedAngle;
  moveServoIfNeeded(closedAngle, F("[lid] init_closed"));
  Serial.println(F("[lid] servo attached D7; closed position set (provisional angle)."));
}

void lidControlUpdate(unsigned long nowMs, bool approachDetected) {
  if (!s_servoReady) {
    return;
  }

  const uint8_t openAngle = clampServoAngle(CONFIG_SERVO_ANGLE_OPEN);
  const uint8_t closedAngle = clampServoAngle(CONFIG_SERVO_ANGLE_CLOSED);

  if (approachDetected) {
    if (s_closeDelayPending) {
      s_closeDelayPending = false;
      Serial.println(F("[lid] close_delay_cancelled approach"));
    }

    if (s_lidState != LidPositionState::Open) {
      s_lidState = LidPositionState::Open;
      moveServoIfNeeded(openAngle, F("[lid] opening"));
      Serial.println(F("[lid] state=OPEN"));
    }
    return;
  }

  if (s_lidState == LidPositionState::Open) {
    if (!s_closeDelayPending) {
      s_closeDelayPending = true;
      s_closeDelayStartMs = nowMs;
      Serial.print(F("[lid] close_delay_start ms="));
      Serial.println(CONFIG_LID_CLOSE_DELAY_MS);
    } else if (closeDelayElapsed(nowMs)) {
      s_closeDelayPending = false;
      s_lidState = LidPositionState::Closed;
      moveServoIfNeeded(closedAngle, F("[lid] closing"));
      Serial.println(F("[lid] state=CLOSED"));
    }
  }
}

LidPositionState lidControlGetLidState() {
  return s_lidState;
}

uint8_t lidControlGetServoTargetAngle() {
  return s_targetAngle;
}

bool lidControlIsCloseDelayPending() {
  return s_closeDelayPending;
}
