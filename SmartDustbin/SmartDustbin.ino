#include <Arduino.h>

#include "application_status.h"
#include "approach_sensor.h"
#include "config.h"
#include "gpio_init.h"
#include "ultrasonic_scheduler.h"
#include "led_status.h"
#include "lid_control.h"
#include "waste_fill_level.h"
#include "waste_sensor.h"

static unsigned long s_lastHeartbeatMs = 0;

static void printBootBanner() {
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F(" Smart Dustbin — WP-7"));
  Serial.println(F(" Application status integration"));
  Serial.println(F("================================"));
  Serial.print(F("Serial baud: "));
  Serial.println(CONFIG_SERIAL_BAUD);
  Serial.print(F("Approach detect <= "));
  Serial.print(CONFIG_APPROACH_DETECT_CM);
  Serial.print(F(" cm | clear >= "));
  Serial.print(CONFIG_APPROACH_CLEAR_CM);
  Serial.println(F(" cm"));
  Serial.print(F("Waste refs (PROVISIONAL): empty="));
  Serial.print(CONFIG_WASTE_EMPTY_REF_CM);
  Serial.print(F(" cm full="));
  Serial.print(CONFIG_WASTE_FULL_REF_CM);
  Serial.println(F(" cm — not physically calibrated"));
  Serial.print(F("Approach sample: "));
  Serial.print(CONFIG_APPROACH_SAMPLE_MS);
  Serial.print(F(" ms | Waste sample: "));
  Serial.print(CONFIG_WASTE_SAMPLE_MS);
  Serial.print(F(" ms | Quiet: "));
  Serial.print(CONFIG_ULTRASONIC_QUIET_MS);
  Serial.println(F(" ms"));
  Serial.print(F("Lid angles (PROVISIONAL): closed="));
  Serial.print(CONFIG_SERVO_ANGLE_CLOSED);
  Serial.print(F(" open="));
  Serial.print(CONFIG_SERVO_ANGLE_OPEN);
  Serial.print(F(" close_delay_ms="));
  Serial.println(CONFIG_LID_CLOSE_DELAY_MS);
  Serial.print(F("LED warn/clear: "));
  Serial.print(CONFIG_WARN_FILL_PERCENT);
  Serial.print(F("% / "));
  Serial.print(CONFIG_WARN_CLEAR_PERCENT);
  Serial.println(F("% (provisional)"));

  if (!wasteFillLevelIsCalibrationValid()) {
    Serial.println(F("[cal] ERROR: empty_ref must be > full_ref"));
  }

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

static void printWasteReading(const WasteSensorReading& r) {
  Serial.print(F("[waste] "));
  Serial.print(F("measurement_valid="));
  Serial.print(r.valid ? 1 : 0);

  if (r.valid) {
    Serial.print(F(" distance_cm="));
    Serial.print(r.distanceCm);
  } else if (r.timedOut) {
    Serial.print(F(" reason=timeout"));
  } else if (r.outOfRange) {
    Serial.print(F(" reason=out_of_range raw_cm="));
    Serial.print(r.distanceCm);
  }

  Serial.print(F(" | cal_valid="));
  Serial.print(wasteFillLevelIsCalibrationValid() ? 1 : 0);

  if (r.valid && !wasteFillLevelIsCalibrationValid()) {
    Serial.print(F(" cal_error=empty_ref_le_full_ref"));
  }

  Serial.print(F(" | fill_pct="));
  uint8_t fillPct = 0;
  if (wasteFillLevelGetLastValidPercent(fillPct)) {
    Serial.print(fillPct);
    Serial.print(F(" fill_ever_valid=1"));
  } else {
    Serial.print(F("none fill_ever_valid="));
    Serial.print(wasteFillLevelHasValidPercent() ? 1 : 0);
  }

  uint16_t lastDist = 0;
  if (wasteSensorGetLastValidDistance(lastDist)) {
    Serial.print(F(" | waste_last_valid_cm="));
    Serial.print(lastDist);
  }

  Serial.println();
}

void setup() {
  Serial.begin(CONFIG_SERIAL_BAUD);
  delay(50);

  gpioInitSafeOutputsAndInputs();
  approachSensorBegin();
  wasteSensorBegin();
  wasteFillLevelBegin();
  ultrasonicSchedulerBegin();
  lidControlBegin();
  ledStatusBegin();

  printBootBanner();
  Serial.println(F("[boot] scheduler + fill + lid + LEDs + app status ready."));
}

void loop() {
  const unsigned long now = millis();

  lidControlUpdate(now, approachSensorIsApproachDetected());

  const UltrasonicMeasurementKind kind = ultrasonicSchedulerTick(now);

  if (kind == UltrasonicMeasurementKind::Approach) {
    ApproachSensorReading reading;
    approachSensorGetReading(reading);
    printApproachReading(reading, approachSensorIsApproachDetected());
  } else if (kind == UltrasonicMeasurementKind::Waste) {
    WasteSensorReading reading;
    wasteSensorGetReading(reading);
    wasteFillLevelApplyReading(reading);
    printWasteReading(reading);
  }

  ledStatusUpdate();

  if (now - s_lastHeartbeatMs >= CONFIG_HEARTBEAT_MS) {
    s_lastHeartbeatMs = now;

    ApplicationStatusSnapshot snapshot;
    applicationStatusGetSnapshot(now, snapshot);
    applicationStatusPrintDiagnostic(snapshot);

    Serial.print(F("[heartbeat] uptime_ms="));
    Serial.print(now);
    Serial.print(F(" approach_detected="));
    Serial.print(approachSensorIsApproachDetected() ? 1 : 0);

    uint16_t validWasteCm = 0;
    if (wasteSensorGetLastValidDistance(validWasteCm)) {
      Serial.print(F(" waste_last_valid_cm="));
      Serial.print(validWasteCm);
    } else {
      Serial.print(F(" waste_last_valid_cm=none"));
    }

    Serial.print(F(" cal_valid="));
    Serial.print(wasteFillLevelIsCalibrationValid() ? 1 : 0);

    uint8_t fillPct = 0;
    if (wasteFillLevelGetLastValidPercent(fillPct)) {
      Serial.print(F(" fill_pct="));
      Serial.print(fillPct);
    } else {
      Serial.print(F(" fill_pct=none"));
    }
    Serial.print(F(" fill_ever_valid="));
    Serial.print(wasteFillLevelHasValidPercent() ? 1 : 0);

    Serial.print(F(" lid="));
    Serial.print(lidControlGetLidState() == LidPositionState::Open ? F("OPEN")
                                                                 : F("CLOSED"));
    Serial.print(F(" servo_target_deg="));
    Serial.print(lidControlGetServoTargetAngle());
    Serial.print(F(" close_pending="));
    Serial.print(lidControlIsCloseDelayPending() ? 1 : 0);

    Serial.print(F(" led="));
    switch (ledStatusGetState()) {
      case LedFillIndicatorState::Normal:
        Serial.print(F("NORMAL"));
        break;
      case LedFillIndicatorState::Warning:
        Serial.print(F("WARNING"));
        break;
      default:
        Serial.print(F("UNKNOWN"));
        break;
    }
    Serial.print(F(" white="));
    Serial.print(ledStatusGetWhiteOn() ? 1 : 0);
    Serial.print(F(" yellow="));
    Serial.print(ledStatusGetYellowOn() ? 1 : 0);

    Serial.println();
  }
}
