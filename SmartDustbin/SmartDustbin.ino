#include <Arduino.h>

#include "approach_sensor.h"
#include "config.h"
#include "gpio_init.h"
#include "pins.h"

static unsigned long s_lastHeartbeatMs = 0;

static void printBootBanner() {
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F(" Smart Dustbin — WP-2"));
  Serial.println(F(" Approach HC-SR04 + Serial"));
  Serial.println(F("================================"));
  Serial.print(F("Serial baud: "));
  Serial.println(CONFIG_SERIAL_BAUD);
  Serial.print(F("Approach detect <= "));
  Serial.print(CONFIG_APPROACH_DETECT_CM);
  Serial.print(F(" cm | clear >= "));
  Serial.print(CONFIG_APPROACH_CLEAR_CM);
  Serial.println(F(" cm"));
  Serial.print(F("Sample interval: "));
  Serial.print(CONFIG_APPROACH_SAMPLE_MS);
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

void setup() {
  Serial.begin(CONFIG_SERIAL_BAUD);
  delay(50);

  gpioInitSafeOutputsAndInputs();
  approachSensorBegin();

  printBootBanner();
  Serial.println(F("[boot] GPIO safe idle + approach sensor ready."));
}

void loop() {
  const unsigned long now = millis();

  if (approachSensorUpdate(now)) {
    ApproachSensorReading reading;
    approachSensorGetReading(reading);
    printApproachReading(reading, approachSensorIsApproachDetected());
  }

  if (now - s_lastHeartbeatMs >= CONFIG_HEARTBEAT_MS) {
    s_lastHeartbeatMs = now;
    Serial.print(F("[heartbeat] uptime_ms="));
    Serial.print(now);
    Serial.print(F(" approach_detected="));
    Serial.println(approachSensorIsApproachDetected() ? 1 : 0);
  }
}
