#include <Arduino.h>

#include "hcsr04_common.h"

Hcsr04Reading hcsr04Measure(uint8_t trigPin, uint8_t echoPin,
                            unsigned long pulseTimeoutUs, uint16_t minValidCm,
                            uint16_t maxValidCm, unsigned long usPerCm) {
  Hcsr04Reading r = {};
  r.valid = false;
  r.timedOut = false;
  r.outOfRange = false;
  r.distanceCm = 0;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  const unsigned long durationUs = pulseIn(echoPin, HIGH, pulseTimeoutUs);

  if (durationUs == 0) {
    r.timedOut = true;
    return r;
  }

  const uint32_t distanceCm = durationUs / usPerCm;

  if (distanceCm < minValidCm || distanceCm > maxValidCm) {
    r.outOfRange = true;
    r.distanceCm = static_cast<uint16_t>(distanceCm);
    return r;
  }

  r.valid = true;
  r.distanceCm = static_cast<uint16_t>(distanceCm);
  return r;
}
