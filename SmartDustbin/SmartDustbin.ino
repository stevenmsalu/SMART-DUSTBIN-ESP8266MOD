#include <Arduino.h>

#include "src/application_status.h"
#include "src/approach_sensor.h"
#include "config.h"
#include "src/gpio_init.h"
#include "src/http_api.h"
#include "src/lid_control.h"
#include "src/white_indicator.h"
#include "src/wifi_manager.h"

static unsigned long s_lastHeartbeatMs = 0;
static unsigned long s_quietUntilMs = 0;
static unsigned long s_nextApproachMs = 0;

static bool runApproachMeasurementIfDue(unsigned long nowMs) {
  if (nowMs < s_quietUntilMs) {
    return false;
  }
  if (nowMs < s_nextApproachMs) {
    return false;
  }

  approachSensorRunMeasurement();
  s_nextApproachMs = millis() + CONFIG_APPROACH_SAMPLE_MS;
  s_quietUntilMs = millis() + CONFIG_ULTRASONIC_QUIET_MS;
  return true;
}

static void printBootBanner() {
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F(" Smart Dustbin — WP-11.2"));
  Serial.println(F(" Approach + lid + white indicator"));
  Serial.println(F(" Dashboard + HTTP API"));
  Serial.println(F("================================"));
  Serial.print(F("Serial baud: "));
  Serial.println(CONFIG_SERIAL_BAUD);
  Serial.print(F("Approach detect <= "));
  Serial.print(CONFIG_APPROACH_DETECT_CM);
  Serial.print(F(" cm | clear >= "));
  Serial.print(CONFIG_APPROACH_CLEAR_CM);
  Serial.println(F(" cm"));
  Serial.print(F("Approach sample: "));
  Serial.print(CONFIG_APPROACH_SAMPLE_MS);
  Serial.print(F(" ms | Quiet: "));
  Serial.print(CONFIG_ULTRASONIC_QUIET_MS);
  Serial.println(F(" ms"));
  Serial.print(F("Lid angles (PROVISIONAL): closed="));
  Serial.print(CONFIG_SERVO_ANGLE_CLOSED);
  Serial.print(F(" open="));
  Serial.print(CONFIG_SERVO_ANGLE_OPEN);
  Serial.print(F(" close_delay_ms="));
  Serial.println(CONFIG_LID_CLOSE_DELAY_MS);

  Serial.println();
}

static void printApproachReading(const ApproachSensorReading& r, bool approach) {
  Serial.print(F("[approach] "));

  if (r.valid) {
    Serial.print(F("distance_cm="));
    Serial.print(r.distanceCm);
    Serial.print(F(" valid=1"));
  } else if (r.timedOut) {
    Serial.print(F("valid=0 reason=timeout"));
  } else if (r.outOfRange) {
    Serial.print(F("valid=0 reason=out_of_range raw_cm="));
    Serial.print(r.distanceCm);
  } else {
    Serial.print(F("valid=0 reason=unknown"));
  }

  Serial.print(F(" | detected="));
  Serial.println(approach ? 1 : 0);
}

void setup() {
  Serial.begin(CONFIG_SERIAL_BAUD);
  delay(50);

  gpioInitSafeOutputsAndInputs();
  approachSensorBegin();
  s_quietUntilMs = 0;
  s_nextApproachMs = 0;
  lidControlBegin();
  whiteIndicatorBegin();
  wifiManagerBegin();
  httpApiBegin();

  printBootBanner();
  Serial.println(F("[boot] subsystems + Wi-Fi + HTTP + dashboard ready."));
}

void loop() {
  const unsigned long now = millis();

  lidControlUpdate(now, approachSensorIsApproachDetected());
  whiteIndicatorUpdate();

  if (runApproachMeasurementIfDue(now)) {
    ApproachSensorReading reading;
    approachSensorGetReading(reading);
    printApproachReading(reading, approachSensorIsApproachDetected());
  }

  wifiManagerUpdate(now);
  httpApiHandleClient();

  if (now - s_lastHeartbeatMs >= CONFIG_HEARTBEAT_MS) {
    s_lastHeartbeatMs = now;

    ApplicationStatusSnapshot snapshot;
    applicationStatusGetSnapshot(now, snapshot);
    applicationStatusPrintDiagnostic(snapshot);

    Serial.print(F("[heartbeat] uptime_ms="));
    Serial.print(now);
    Serial.print(F(" approach_detected="));
    Serial.print(approachSensorIsApproachDetected() ? 1 : 0);

    Serial.print(F(" lid="));
    Serial.print(lidControlGetLidState() == LidPositionState::Open ? F("OPEN")
                                                                 : F("CLOSED"));
    Serial.print(F(" servo_target_deg="));
    Serial.print(lidControlGetServoTargetAngle());
    Serial.print(F(" close_pending="));
    Serial.print(lidControlIsCloseDelayPending() ? 1 : 0);

    Serial.print(F(" white="));
    Serial.print(whiteIndicatorIsOn() ? 1 : 0);

    Serial.println();
  }
}