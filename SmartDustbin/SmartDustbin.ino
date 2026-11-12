#include <Arduino.h>

#include "approach_sensor.h"
#include "config.h"
#include "gpio_init.h"
#include "ultrasonic_scheduler.h"
#include "waste_sensor.h"

static unsigned long s_lastHeartbeatMs = 0;

static void printBootBanner() {
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F(" Smart Dustbin — WP-3"));
  Serial.println(F(" Dual HC-SR04 (scheduled)"));
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
  Serial.print(F(" ms | Waste sample: "));
  Serial.print(CONFIG_WASTE_SAMPLE_MS);
  Serial.print(F(" ms | Quiet: "));
  Serial.print(CONFIG_ULTRASONIC_QUIET_MS);
  Serial.println(F(" ms"));
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

  if (r.valid) {
    Serial.print(F("distance_cm="));
    Serial.print(r.distanceCm);
    Serial.println(F(" valid=1"));
  } else if (r.timedOut) {
    Serial.println(F("valid=0 reason=timeout"));
  } else if (r.outOfRange) {
    Serial.print(F("valid=0 reason=out_of_range raw_cm="));
    Serial.println(r.distanceCm);
  } else {
    Serial.println(F("valid=0 reason=unknown"));
  }
}

void setup() {
  Serial.begin(CONFIG_SERIAL_BAUD);
  delay(50);

  gpioInitSafeOutputsAndInputs();
  approachSensorBegin();
  wasteSensorBegin();
  ultrasonicSchedulerBegin();

  printBootBanner();
  Serial.println(F("[boot] GPIO idle + dual ultrasonic scheduler ready."));
}

void loop() {
  const unsigned long now = millis();

  const UltrasonicMeasurementKind kind = ultrasonicSchedulerTick(now);

  if (kind == UltrasonicMeasurementKind::Approach) {
    ApproachSensorReading reading;
    approachSensorGetReading(reading);
    printApproachReading(reading, approachSensorIsApproachDetected());
  } else if (kind == UltrasonicMeasurementKind::Waste) {
    WasteSensorReading reading;
    wasteSensorGetReading(reading);
    printWasteReading(reading);
  }

  if (now - s_lastHeartbeatMs >= CONFIG_HEARTBEAT_MS) {
    s_lastHeartbeatMs = now;
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
    Serial.println();
  }
}
